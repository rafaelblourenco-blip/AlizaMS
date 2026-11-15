#!/usr/bin/env python3
"""
e2e_engine.py — Core E2E Testing Engine for AlizaMS DICOM Viewer.
Provides safe process lifecycle management, PID isolation, Win32 API / UIAutomation,
screen capture / pixel assertions, and structured reporting.
"""

import os
import sys
import time
import shutil
import subprocess
import json
import ctypes
from ctypes import wintypes
from dataclasses import dataclass, asdict
from typing import List, Optional, Tuple, Dict, Any
from PIL import Image, ImageGrab
import numpy as np
import psutil

# Safe PID blacklist (user's active instance)
PROTECTED_PIDS = {3372}

# Win32 Constants
WM_CLOSE = 0x0010
WM_DROPFILES = 0x0233
WM_MOUSEWHEEL = 0x020A
WM_MOUSEMOVE = 0x0200
WM_LBUTTONDOWN = 0x0201
WM_LBUTTONUP = 0x0202
WM_RBUTTONDOWN = 0x0204
WM_RBUTTONUP = 0x0205
WM_MBUTTONDOWN = 0x0207
WM_MBUTTONUP = 0x0208
WM_KEYDOWN = 0x0100
WM_KEYUP = 0x0101
WM_COMMAND = 0x0111

MK_LBUTTON = 0x0001
MK_RBUTTON = 0x0002
MK_MBUTTON = 0x0010

GMEM_MOVEABLE = 0x0002
GMEM_ZEROINIT = 0x0040

user32 = ctypes.windll.user32
kernel32 = ctypes.windll.kernel32
gdi32 = ctypes.windll.gdi32

PW_CLIENTONLY = 1
PW_RENDERFULLCONTENT = 2

WNDENUMPROC = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)

user32.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
user32.PostMessageW.restype = wintypes.BOOL

kernel32.GlobalAlloc.restype = wintypes.HGLOBAL
kernel32.GlobalAlloc.argtypes = [wintypes.UINT, ctypes.c_size_t]
kernel32.GlobalLock.restype = ctypes.c_void_p
kernel32.GlobalLock.argtypes = [wintypes.HGLOBAL]
kernel32.GlobalUnlock.restype = wintypes.BOOL
kernel32.GlobalUnlock.argtypes = [wintypes.HGLOBAL]

class DROPFILES(ctypes.Structure):
    _fields_ = [
        ("pFiles", wintypes.DWORD),
        ("pt", wintypes.POINT),
        ("fNC", wintypes.BOOL),
        ("fWide", wintypes.BOOL),
    ]

class RECT(ctypes.Structure):
    _fields_ = [
        ("left", wintypes.LONG),
        ("top", wintypes.LONG),
        ("right", wintypes.LONG),
        ("bottom", wintypes.LONG),
    ]

class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [
        ("biSize", wintypes.DWORD),
        ("biWidth", wintypes.LONG),
        ("biHeight", wintypes.LONG),
        ("biPlanes", wintypes.WORD),
        ("biBitCount", wintypes.WORD),
        ("biCompression", wintypes.DWORD),
        ("biSizeImage", wintypes.DWORD),
        ("biXPelsPerMeter", wintypes.LONG),
        ("biYPelsPerMeter", wintypes.LONG),
        ("biClrUsed", wintypes.DWORD),
        ("biClrImportant", wintypes.DWORD),
    ]

def resolve_binary_path(custom_path: Optional[str] = None) -> str:
    """Finds the AlizaMS executable to test."""
    if custom_path:
        if os.path.isfile(custom_path):
            return os.path.abspath(custom_path)
        raise FileNotFoundError(f"Specified AlizaMS binary not found: {custom_path}")
        
    candidates = [
        r"C:\Projeto\Dicom Viewer\aliza-radiant-windows-x64-Build-2\aliza-radiant-windows-x64\alizams.exe",
        r"C:\Projeto\Dicom Viewer\aliza-radiant-windows-x64-Build-2\aliza-radiant-windows-x64\aliza.exe",
        r"c:\Projeto\aliza-radiant-windows-x64\aliza-radiant-windows-x64\alizams.exe",
        r"c:\Projeto\AlizaMS\build\bin\Release\alizams.exe",
    ]
    
    for c in candidates:
        if os.path.isfile(c):
            return os.path.abspath(c)
            
    raise FileNotFoundError(f"Could not locate AlizaMS binary in candidates: {candidates}")

class AlizaProcess:
    """
    Manages AlizaMS process lifecycle with strict PID isolation.
    Guarantees user's active instances (PID 3372) are NEVER touched.
    """
    def __init__(self, args: Optional[List[str]] = None, binary_path: Optional[str] = None, nogl: bool = True):
        self.binary_path = resolve_binary_path(binary_path)
        self.args = args or []
        self.nogl = nogl
        self.proc: Optional[subprocess.Popen] = None
        self.child_pid: Optional[int] = None
        self.main_hwnd: Optional[int] = None
        self.ini_backup_path: Optional[str] = None
        self.ini_existed_originally: bool = True
        self.appdata_ini = os.path.expandvars(r"%APPDATA%\Aliza\AlizaMS.ini")
        
    def __enter__(self):
        self.start()
        return self
        
    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()
        
    def _backup_settings(self):
        try:
            self.ini_backup_path = self.appdata_ini + ".e2e_backup"
            backup_already_exists = os.path.isfile(self.ini_backup_path)
            self.ini_existed_originally = os.path.isfile(self.appdata_ini) or backup_already_exists
            
            # Do NOT overwrite existing backup from a prior crashed run
            if os.path.isfile(self.appdata_ini) and not backup_already_exists:
                shutil.copy2(self.appdata_ini, self.ini_backup_path)
        except Exception as e:
            print(f"[Warning] Failed to backup AlizaMS.ini: {e}")
            
    def _restore_settings(self):
        try:
            if self.ini_backup_path and os.path.isfile(self.ini_backup_path):
                shutil.copy2(self.ini_backup_path, self.appdata_ini)
                os.remove(self.ini_backup_path)
                self.ini_backup_path = None
            elif not getattr(self, "ini_existed_originally", True) and os.path.isfile(self.appdata_ini):
                # Clean up newly generated INI if user had none originally
                os.remove(self.appdata_ini)
        except Exception as e:
            print(f"[Warning] Failed to restore AlizaMS.ini: {e}")

    def start(self):
        self._backup_settings()
        cmd = [self.binary_path]
        if self.nogl and "-nogl" not in self.args and "--nogl" not in self.args:
            cmd.append("-nogl")
        cmd.extend(self.args)
        
        self.proc = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        self.child_pid = self.proc.pid
        
        # Rigorous PID isolation assertion
        if self.child_pid in PROTECTED_PIDS:
            # Under NO circumstances invoke self.proc.kill() if PID matches protected set!
            bad_pid = self.child_pid
            self.proc = None
            self.child_pid = None
            raise RuntimeError(f"FATAL: Spawned PID matches protected PID {bad_pid}!")
            
        return self

    def wait_for_window(self, title_substr: str = "Aliza MS", timeout: float = 8.0) -> int:
        """Polls for top-level window belonging exclusively to child_pid."""
        start_time = time.time()
        while time.time() - start_time < timeout:
            if self.proc.poll() is not None:
                raise RuntimeError(f"Process terminated prematurely with code {self.proc.returncode}")
                
            windows = Win32Helper.find_windows_for_pid(self.child_pid)
            for h, title in windows:
                if title_substr.lower() in title.lower():
                    rect = Win32Helper.get_window_rect(h)
                    if rect[4] > 100 and rect[5] > 100:
                        self.main_hwnd = h
                        return h
            time.sleep(0.1)
            
        raise TimeoutError(f"Timed out waiting for window containing '{title_substr}' (PID: {self.child_pid})")

    def probe_mpr_window(self, timeout: float = 0.5) -> Optional[int]:
        """Probes whether a dedicated MPR overlay window exists or appears on VK_F4."""
        if not self.child_pid:
            return None
        for h, title in Win32Helper.find_windows_for_pid(self.child_pid):
            if "mpr" in title.lower() or "multi-planar" in title.lower():
                return h
        if self.main_hwnd:
            Win32Helper.send_key(self.main_hwnd, 0x73)  # VK_F4
            t_end = time.time() + timeout
            while time.time() < t_end:
                for h, title in Win32Helper.find_windows_for_pid(self.child_pid):
                    if "mpr" in title.lower() or "multi-planar" in title.lower():
                        return h
                time.sleep(0.05)
        return None

    def is_running(self) -> bool:
        return self.proc is not None and self.proc.poll() is None

    def close(self, timeout: float = 5.0) -> int:
        """Sends WM_CLOSE for graceful exit. Fallback to process kill strictly for child_pid."""
        if not self.is_running():
            self._restore_settings()
            return self.proc.returncode if self.proc else 0
            
        try:
            if self.main_hwnd and user32.IsWindow(self.main_hwnd):
                user32.PostMessageW(self.main_hwnd, WM_CLOSE, 0, 0)
                
            start_t = time.time()
            while time.time() - start_t < timeout:
                if self.proc.poll() is not None:
                    break
                time.sleep(0.1)
                
            if self.proc.poll() is None:
                # Force kill ONLY child_pid
                if self.child_pid and self.child_pid not in PROTECTED_PIDS:
                    self.proc.kill()
                    self.proc.wait(timeout=2.0)
        finally:
            self._restore_settings()
            
        return self.proc.returncode if self.proc else 0

class Win32Helper:
    """Win32 API wrappers for window discovery, geometry measurement, and event simulation."""
    user32 = user32
    kernel32 = kernel32
    gdi32 = gdi32
    
    @staticmethod
    def find_windows_for_pid(target_pid: int) -> List[Tuple[int, str]]:
        windows = []
        def enum_cb(h, l):
            pid = wintypes.DWORD()
            user32.GetWindowThreadProcessId(h, ctypes.byref(pid))
            if pid.value == target_pid:
                length = user32.GetWindowTextLengthW(h)
                buf = ctypes.create_unicode_buffer(length + 1)
                user32.GetWindowTextW(h, buf, length + 1)
                if user32.IsWindowVisible(h):
                    windows.append((h, buf.value))
            return True
        user32.EnumWindows(WNDENUMPROC(enum_cb), 0)
        return windows

    @staticmethod
    def get_window_rect(hwnd: int) -> Tuple[int, int, int, int, int, int]:
        """Returns (left, top, right, bottom, width, height) in screen coordinates."""
        r = RECT()
        user32.GetWindowRect(hwnd, ctypes.byref(r))
        w = r.right - r.left
        h = r.bottom - r.top
        return (r.left, r.top, r.right, r.bottom, w, h)

    @staticmethod
    def get_client_rect(hwnd: int) -> Tuple[int, int, int, int, int, int]:
        """Returns client area rectangle (left, top, right, bottom, width, height)."""
        r = RECT()
        user32.GetClientRect(hwnd, ctypes.byref(r))
        w = r.right - r.left
        h = r.bottom - r.top
        return (r.left, r.top, r.right, r.bottom, w, h)

    @staticmethod
    def get_window_text(hwnd: int) -> str:
        length = user32.GetWindowTextLengthW(hwnd)
        buf = ctypes.create_unicode_buffer(length + 1)
        user32.GetWindowTextW(hwnd, buf, length + 1)
        return buf.value

    @staticmethod
    def get_class_name(hwnd: int) -> str:
        buf = ctypes.create_unicode_buffer(256)
        user32.GetClassNameW(hwnd, buf, 256)
        return buf.value

    @staticmethod
    def enum_child_windows(parent_hwnd: int) -> List[int]:
        children = []
        def child_cb(h, l):
            children.append(h)
            return True
        user32.EnumChildWindows(parent_hwnd, WNDENUMPROC(child_cb), 0)
        return children

    @staticmethod
    def send_mouse_wheel(hwnd: int, delta: int, x: int = 400, y: int = 300):
        """Dispatches WM_MOUSEWHEEL event with given delta (standard notch = 120) converted to screen coordinates."""
        r = RECT()
        user32.GetWindowRect(hwnd, ctypes.byref(r))
        screen_x = r.left + x
        screen_y = r.top + y
        try:
            user32.SetCursorPos(screen_x, screen_y)
        except Exception:
            pass
        wparam = (delta << 16) & 0xFFFFFFFF
        lparam = ((screen_y & 0xFFFF) << 16) | (screen_x & 0xFFFF)
        user32.PostMessageW(hwnd, WM_MOUSEWHEEL, wparam, lparam)
        time.sleep(0.01)
        # Ensure interactive viewport canvas update
        lp1 = (y << 16) | (x & 0xFFFF)
        lp2 = ((y + 2) << 16) | ((x + 2) & 0xFFFF)
        user32.PostMessageW(hwnd, WM_RBUTTONDOWN, MK_RBUTTON, lp1)
        user32.PostMessageW(hwnd, WM_MOUSEMOVE, MK_RBUTTON, lp2)
        user32.PostMessageW(hwnd, WM_RBUTTONUP, 0, lp2)

    @staticmethod
    def send_click(hwnd: int, x: int, y: int, button: str = "left"):
        lparam = (y << 16) | (x & 0xFFFF)
        if button == "left":
            user32.PostMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lparam)
            time.sleep(0.02)
            user32.PostMessageW(hwnd, WM_LBUTTONUP, 0, lparam)
        elif button == "right":
            user32.PostMessageW(hwnd, WM_RBUTTONDOWN, MK_RBUTTON, lparam)
            time.sleep(0.02)
            user32.PostMessageW(hwnd, WM_RBUTTONUP, 0, lparam)
        elif button == "middle":
            user32.PostMessageW(hwnd, WM_MBUTTONDOWN, MK_MBUTTON, lparam)
            time.sleep(0.02)
            user32.PostMessageW(hwnd, WM_MBUTTONUP, 0, lparam)

    @staticmethod
    def send_drag(hwnd: int, x1: int, y1: int, x2: int, y2: int, button: str = "left", steps: int = 5):
        """Simulates dragging the mouse with a button depressed."""
        lparam1 = (y1 << 16) | (x1 & 0xFFFF)
        btn_down = WM_LBUTTONDOWN if button == "left" else WM_RBUTTONDOWN
        btn_up = WM_LBUTTONUP if button == "left" else WM_RBUTTONUP
        mk_flag = MK_LBUTTON if button == "left" else MK_RBUTTON
        
        user32.PostMessageW(hwnd, btn_down, mk_flag, lparam1)
        time.sleep(0.02)
        for i in range(1, steps + 1):
            xi = int(x1 + (x2 - x1) * i / steps)
            yi = int(y1 + (y2 - y1) * i / steps)
            lp = (yi << 16) | (xi & 0xFFFF)
            user32.PostMessageW(hwnd, 0x0200, mk_flag, lp)  # WM_MOUSEMOVE
            time.sleep(0.01)
        lparam2 = (y2 << 16) | (x2 & 0xFFFF)
        user32.PostMessageW(hwnd, btn_up, 0, lparam2)

    @staticmethod
    def send_key(hwnd: int, vk_code: int):
        user32.PostMessageW(hwnd, WM_KEYDOWN, vk_code, 0)
        time.sleep(0.02)
        user32.PostMessageW(hwnd, WM_KEYUP, vk_code, 0)

    @staticmethod
    def post_drop_files(hwnd: int, file_paths: List[str], x: int = 300, y: int = 300):
        """Simulates drag-and-drop of files using Win32 WM_DROPFILES."""
        # Calculate size of file list buffer (UTF-16 double-null terminated)
        encoded_paths = [os.path.abspath(p) + "\0" for p in file_paths]
        joined = "".join(encoded_paths) + "\0"
        byte_data = joined.encode("utf-16le")
        
        df_size = ctypes.sizeof(DROPFILES)
        total_size = df_size + len(byte_data)
        
        h_global = kernel32.GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, total_size)
        ptr = kernel32.GlobalLock(h_global)
        
        df = DROPFILES()
        df.pFiles = df_size
        df.pt.x = x
        df.pt.y = y
        df.fNC = False
        df.fWide = True
        
        ctypes.memmove(ptr, ctypes.byref(df), df_size)
        ctypes.memmove(ptr + df_size, byte_data, len(byte_data))
        kernel32.GlobalUnlock(h_global)
        
        user32.PostMessageW(hwnd, WM_DROPFILES, h_global, 0)

    @staticmethod
    def get_pixel_diff(img1: Optional[Image.Image], img2: Optional[Image.Image]) -> float:
        """Computes mean absolute pixel difference between two PIL images."""
        if img1 is None or img2 is None:
            return 0.0
        a1 = np.array(img1, dtype=float)
        a2 = np.array(img2, dtype=float)
        if a1.shape != a2.shape:
            min_h = min(a1.shape[0], a2.shape[0])
            min_w = min(a1.shape[1], a2.shape[1])
            a1 = a1[:min_h, :min_w]
            a2 = a2[:min_h, :min_w]
        return float(np.abs(a1 - a2).mean())

class ScreenCapture:
    """Screen grabbing and visual verification via native Win32 PrintWindow."""
    
    @staticmethod
    def grab_window(hwnd: int, client_only: bool = True) -> Optional[Image.Image]:
        if not user32.IsWindow(hwnd):
            return None
            
        r = wintypes.RECT()
        if client_only:
            user32.GetClientRect(hwnd, ctypes.byref(r))
            w = r.right - r.left
            h = r.bottom - r.top
            hdc_window = user32.GetDC(hwnd)
            pw_flags = PW_CLIENTONLY
        else:
            user32.GetWindowRect(hwnd, ctypes.byref(r))
            w = r.right - r.left
            h = r.bottom - r.top
            hdc_window = user32.GetWindowDC(hwnd)
            pw_flags = PW_RENDERFULLCONTENT
            
        if w <= 0 or h <= 0:
            if hdc_window:
                user32.ReleaseDC(hwnd, hdc_window)
            return None
            
        hdc_mem = None
        hbm = None
        old_bm = None
        try:
            hdc_mem = gdi32.CreateCompatibleDC(hdc_window)
            hbm = gdi32.CreateCompatibleBitmap(hdc_window, w, h)
            old_bm = gdi32.SelectObject(hdc_mem, hbm)
            
            res = user32.PrintWindow(hwnd, hdc_mem, pw_flags)
            if not res and client_only:
                res = user32.PrintWindow(hwnd, hdc_mem, 0)
                
            header = BITMAPINFOHEADER()
            header.biSize = ctypes.sizeof(BITMAPINFOHEADER)
            header.biWidth = w
            header.biHeight = -h  # top-down DIB
            header.biPlanes = 1
            header.biBitCount = 32
            header.biCompression = 0
            
            buf = (ctypes.c_ubyte * (w * h * 4))()
            gdi32.GetDIBits(hdc_mem, hbm, 0, h, ctypes.byref(buf), ctypes.byref(header), 0)
            
            arr = np.frombuffer(buf, dtype=np.uint8).reshape((h, w, 4))
            rgb = arr[:, :, [2, 1, 0]]  # BGRA to RGB
            return Image.fromarray(rgb)
        except Exception:
            try:
                rect = RECT()
                user32.GetWindowRect(hwnd, ctypes.byref(rect))
                bbox = (rect.left, rect.top, rect.right, rect.bottom)
                return ImageGrab.grab(bbox=bbox)
            except Exception:
                return None
        finally:
            if hdc_mem and old_bm:
                gdi32.SelectObject(hdc_mem, old_bm)
            if hbm:
                gdi32.DeleteObject(hbm)
            if hdc_mem:
                gdi32.DeleteDC(hdc_mem)
            if hdc_window:
                user32.ReleaseDC(hwnd, hdc_window)

    @staticmethod
    def save_screenshot(img: Image.Image, name: str) -> str:
        script_dir = os.path.dirname(os.path.abspath(__file__))
        shots_dir = os.path.abspath(os.path.join(script_dir, "..", "reports", "screenshots"))
        os.makedirs(shots_dir, exist_ok=True)
        path = os.path.join(shots_dir, f"{name}.png")
        img.save(path)
        return path

@dataclass
class TestCaseResult:
    test_id: str
    feature_id: int
    name: str
    status: str  # "PASS", "FAIL", "SKIP"
    duration_sec: float
    message: str = ""
    details: Dict[str, Any] = None

class TestReporter:
    def __init__(self, tier_name: str):
        self.tier_name = tier_name
        self.results: List[TestCaseResult] = []
        self.start_time = time.time()
        
    def add_result(self, res: TestCaseResult):
        self.results.append(res)
        tag = "[PASS]" if res.status == "PASS" else "[FAIL]" if res.status == "FAIL" else "[SKIP]"
        print(f"  {tag} {res.test_id}: {res.name} ({res.duration_sec:.2f}s) - {res.message}")

    def summary(self) -> Dict[str, Any]:
        total = len(self.results)
        passed = sum(1 for r in self.results if r.status == "PASS")
        failed = sum(1 for r in self.results if r.status == "FAIL")
        skipped = sum(1 for r in self.results if r.status == "SKIP")
        elapsed = time.time() - self.start_time
        return {
            "tier": self.tier_name,
            "total": total,
            "passed": passed,
            "failed": failed,
            "skipped": skipped,
            "duration_sec": elapsed,
            "pass_rate": (passed / total * 100.0) if total > 0 else 0.0,
            "cases": [asdict(r) for r in self.results]
        }
