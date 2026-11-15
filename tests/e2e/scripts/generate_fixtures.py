#!/usr/bin/env python3
"""
generate_fixtures.py — Deterministic synthetic DICOM dataset & archive generator for AlizaMS E2E tests.
Uses pydicom and numpy to create calibrated geometric phantoms and edge-case datasets.
"""

import os
import sys
import shutil
import zipfile
import numpy as np
import pydicom
from pydicom.dataset import Dataset, FileDataset, FileMetaDataset
from pydicom.uid import ExplicitVRLittleEndian, CTImageStorage, MRImageStorage, SecondaryCaptureImageStorage

UID_ROOT = "1.2.826.0.1.3680043.8.498"

def get_fixtures_dir():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    return os.path.abspath(os.path.join(script_dir, "..", "fixtures"))

def create_ct_dataset(study_uid, series_uid, instance_num, z_pos, rows=512, cols=512,
                      pixel_spacing=(0.5, 0.5), slice_thickness=2.0,
                      patient_name="SYNTHETIC^PATIENT", patient_id="E2E_P001",
                      study_desc="E2E Study", series_desc="CT Series",
                      wc=40, ww=400, phantom_type="sphere",
                      sphere_radius=60, sphere_center_z=0.0):
    sop_uid = f"{series_uid}.{instance_num}"
    file_meta = FileMetaDataset()
    file_meta.MediaStorageSOPClassUID = CTImageStorage
    file_meta.MediaStorageSOPInstanceUID = sop_uid
    file_meta.TransferSyntaxUID = ExplicitVRLittleEndian
    file_meta.ImplementationClassUID = f"{UID_ROOT}.1"
    
    ds = FileDataset(None, {}, file_meta=file_meta, preamble=b"\0" * 128)
    ds.is_little_endian = True
    ds.is_implicit_VR = False
    
    ds.PatientName = patient_name
    ds.PatientID = patient_id
    ds.Modality = "CT"
    ds.StudyInstanceUID = study_uid
    ds.SeriesInstanceUID = series_uid
    ds.SOPInstanceUID = sop_uid
    ds.SOPClassUID = CTImageStorage
    ds.StudyDate = "20261114"
    ds.SeriesDate = "20261114"
    ds.ContentDate = "20261114"
    ds.StudyTime = "120000"
    ds.SeriesTime = "120000"
    ds.ContentTime = "120000"
    ds.StudyDescription = study_desc
    ds.SeriesDescription = series_desc
    ds.InstanceNumber = instance_num
    
    ds.Rows = rows
    ds.Columns = cols
    ds.PixelSpacing = [float(pixel_spacing[0]), float(pixel_spacing[1])]
    ds.SliceThickness = float(slice_thickness)
    ds.ImagePositionPatient = [0.0, 0.0, float(z_pos)]
    ds.ImageOrientationPatient = [1.0, 0.0, 0.0, 0.0, 1.0, 0.0]
    
    ds.SamplesPerPixel = 1
    ds.PhotometricInterpretation = "MONOCHROME2"
    ds.BitsAllocated = 16
    ds.BitsStored = 16
    ds.HighBit = 15
    ds.PixelRepresentation = 1  # signed 16-bit
    ds.RescaleIntercept = -1024
    ds.RescaleSlope = 1
    ds.WindowCenter = wc
    ds.WindowWidth = ww

    # Create pixel data
    # Raw value = HU - RescaleIntercept = HU + 1024
    y, x = np.ogrid[:rows, :cols]
    cx, cy = cols / 2.0, rows / 2.0
    
    # Default air (-1000 HU -> raw 24)
    raw_pixels = np.full((rows, cols), 24, dtype=np.int16)
    
    if phantom_type == "sphere":
        # 3D sphere phantom centered at (cx, cy, sphere_center_z)
        # Coordinates in physical mm
        dx, dy = pixel_spacing[0], pixel_spacing[1]
        phys_x = (x - cx) * dx
        phys_y = (y - cy) * dy
        phys_z = z_pos - sphere_center_z
        r_sq = phys_x**2 + phys_y**2 + phys_z**2
        
        # Outer bone ring: 1000 HU -> raw 2024
        bone_mask = r_sq < (sphere_radius**2)
        raw_pixels[bone_mask] = 1000 + 1024
        
        # Inner soft tissue: 50 HU -> raw 1074
        tissue_mask = r_sq < ((sphere_radius * 0.75)**2)
        raw_pixels[tissue_mask] = 50 + 1024
        
        # Central water core: 0 HU -> raw 1024
        core_mask = r_sq < ((sphere_radius * 0.3)**2)
        raw_pixels[core_mask] = 0 + 1024
        
    elif phantom_type == "calibrated_2d":
        # Calibrated 50mm disc and 20x20mm square
        dx, dy = pixel_spacing[0], pixel_spacing[1]
        phys_x = (x - cx) * dx
        phys_y = (y - cy) * dy
        
        # Background soft tissue ellipse
        ellipse_mask = (phys_x / 100.0)**2 + (phys_y / 80.0)**2 < 1.0
        raw_pixels[ellipse_mask] = 40 + 1024
        
        # 50mm diameter circle (radius 25mm) centered at (-40mm, 0)
        disc_r_sq = (phys_x + 40.0)**2 + phys_y**2
        disc_mask = disc_r_sq < (25.0**2)
        raw_pixels[disc_mask] = 300 + 1024
        
        # 20mm x 20mm square centered at (+40mm, 0)
        square_mask = (np.abs(phys_x - 40.0) <= 10.0) & (np.abs(phys_y) <= 10.0)
        raw_pixels[square_mask] = 600 + 1024
        
    elif phantom_type == "cylinder":
        # Cylindrical phantom across slices
        dx, dy = pixel_spacing[0], pixel_spacing[1]
        phys_x = (x - cx) * dx
        phys_y = (y - cy) * dy
        r_sq = phys_x**2 + phys_y**2
        cyl_mask = r_sq < (70.0**2)
        raw_pixels[cyl_mask] = 100 + 1024
        inner_cyl = r_sq < (35.0**2)
        raw_pixels[inner_cyl] = 30 + 1024

    ds.PixelData = raw_pixels.tobytes()
    return ds

def create_mr_dataset(study_uid, series_uid, instance_num, rows=256, cols=256):
    sop_uid = f"{series_uid}.{instance_num}"
    file_meta = FileMetaDataset()
    file_meta.MediaStorageSOPClassUID = MRImageStorage
    file_meta.MediaStorageSOPInstanceUID = sop_uid
    file_meta.TransferSyntaxUID = ExplicitVRLittleEndian
    file_meta.ImplementationClassUID = f"{UID_ROOT}.1"
    
    ds = FileDataset(None, {}, file_meta=file_meta, preamble=b"\0" * 128)
    ds.is_little_endian = True
    ds.is_implicit_VR = False
    
    ds.PatientName = "SYNTHETIC^MR_BRAIN"
    ds.PatientID = "E2E_P002"
    ds.Modality = "MR"
    ds.StudyInstanceUID = study_uid
    ds.SeriesInstanceUID = series_uid
    ds.SOPInstanceUID = sop_uid
    ds.SOPClassUID = MRImageStorage
    ds.StudyDate = "20261114"
    ds.SeriesDate = "20261114"
    ds.ContentDate = "20261114"
    ds.StudyDescription = "Synthetic MR Brain Exam"
    ds.SeriesDescription = "T1 Axial 256"
    ds.InstanceNumber = instance_num
    
    ds.Rows = rows
    ds.Columns = cols
    ds.PixelSpacing = [1.0, 1.0]
    ds.SliceThickness = 3.0
    ds.ImagePositionPatient = [0.0, 0.0, 0.0]
    ds.ImageOrientationPatient = [1.0, 0.0, 0.0, 0.0, 1.0, 0.0]
    
    ds.SamplesPerPixel = 1
    ds.PhotometricInterpretation = "MONOCHROME2"
    ds.BitsAllocated = 16
    ds.BitsStored = 16
    ds.HighBit = 15
    ds.PixelRepresentation = 0  # unsigned 16-bit
    ds.WindowCenter = 500
    ds.WindowWidth = 1000
    
    y, x = np.ogrid[:rows, :cols]
    cx, cy = cols / 2.0, rows / 2.0
    r_sq = (x - cx)**2 + (y - cy)**2
    
    pixels = np.zeros((rows, cols), dtype=np.uint16)
    # Skull
    pixels[r_sq < 90**2] = 200
    # Brain tissue
    pixels[r_sq < 80**2] = 600
    # Ventricles
    v1 = ((x - (cx - 15)) / 8.0)**2 + ((y - cy) / 25.0)**2 < 1.0
    v2 = ((x - (cx + 15)) / 8.0)**2 + ((y - cy) / 25.0)**2 < 1.0
    pixels[v1 | v2] = 100
    
    ds.PixelData = pixels.tobytes()
    return ds

def generate_all_fixtures():
    fixtures_dir = get_fixtures_dir()
    print(f"Generating fixtures in {fixtures_dir}...")
    
    # 1. Single slice fixtures
    single_dir = os.path.join(fixtures_dir, "dicom", "single_slice")
    os.makedirs(single_dir, exist_ok=True)
    
    ct_single = create_ct_dataset(
        study_uid=f"{UID_ROOT}.1.1",
        series_uid=f"{UID_ROOT}.1.1.1",
        instance_num=1,
        z_pos=0.0,
        rows=512,
        cols=512,
        pixel_spacing=(0.5, 0.5),
        phantom_type="calibrated_2d"
    )
    ct_single.save_as(os.path.join(single_dir, "ct_axial_512.dcm"), write_like_original=False)
    
    mr_single = create_mr_dataset(
        study_uid=f"{UID_ROOT}.1.2",
        series_uid=f"{UID_ROOT}.1.2.1",
        instance_num=1,
        rows=256,
        cols=256
    )
    mr_single.save_as(os.path.join(single_dir, "mr_brain_256.dcm"), write_like_original=False)
    
    # Copy repo's sample US and SR if present
    repo_root = os.path.abspath(os.path.join(fixtures_dir, "..", ".."))
    sample_us = os.path.join(repo_root, "package", "archive", "usr", "share", "alizams", "datasets", "001US", "TESTXX02.dcm")
    sample_sr = os.path.join(repo_root, "package", "archive", "usr", "share", "alizams", "datasets", "001US", "TESTXX04_SR.dcm")
    if os.path.exists(sample_us):
        shutil.copy2(sample_us, os.path.join(single_dir, "us_cardiac.dcm"))
    if os.path.exists(sample_sr):
        shutil.copy2(sample_sr, os.path.join(single_dir, "sr_report.dcm"))
        
    print("  [x] Single-slice fixtures created.")

    # 2. Multi-slice series
    # 2.1 30-slice CT Head with 3D sphere phantom
    head_dir = os.path.join(fixtures_dir, "dicom", "multi_slice_series", "ct_head_30slices")
    os.makedirs(head_dir, exist_ok=True)
    st_head = f"{UID_ROOT}.2.1"
    ser_head = f"{UID_ROOT}.2.1.1"
    for i in range(1, 31):
        z = (i - 15.5) * 2.0  # z from -29mm to +29mm
        ds = create_ct_dataset(
            study_uid=st_head,
            series_uid=ser_head,
            instance_num=i,
            z_pos=z,
            rows=256,
            cols=256,
            pixel_spacing=(1.0, 1.0),
            slice_thickness=2.0,
            phantom_type="sphere",
            sphere_radius=25.0
        )
        ds.save_as(os.path.join(head_dir, f"slice_{i:03d}.dcm"), write_like_original=False)
    print("  [x] 30-slice CT head volume created.")

    # 2.2 100-slice CT Thorax for fluid scroll benchmark
    thorax_dir = os.path.join(fixtures_dir, "dicom", "multi_slice_series", "ct_thorax_100slices")
    os.makedirs(thorax_dir, exist_ok=True)
    st_thorax = f"{UID_ROOT}.2.2"
    ser_thorax = f"{UID_ROOT}.2.2.1"
    for i in range(1, 101):
        z = (i - 50.5) * 1.0
        ds = create_ct_dataset(
            study_uid=st_thorax,
            series_uid=ser_thorax,
            instance_num=i,
            z_pos=z,
            rows=128,  # compact 128x128 for rapid high-count generation
            cols=128,
            pixel_spacing=(1.5, 1.5),
            slice_thickness=1.0,
            phantom_type="cylinder"
        )
        ds.save_as(os.path.join(thorax_dir, f"slice_{i:03d}.dcm"), write_like_original=False)
    print("  [x] 100-slice CT thorax volume created.")

    # 3. Multi-series study (Scout, Soft Tissue, Bone)
    multi_study_dir = os.path.join(fixtures_dir, "dicom", "multi_series_study", "study_multiseries")
    os.makedirs(multi_study_dir, exist_ok=True)
    st_multi = f"{UID_ROOT}.3.1"
    
    # 3.1 Scout
    scout_dir = os.path.join(multi_study_dir, "series_1_scout")
    os.makedirs(scout_dir, exist_ok=True)
    ds_scout = create_ct_dataset(
        study_uid=st_multi,
        series_uid=f"{st_multi}.1",
        instance_num=1,
        z_pos=0.0,
        rows=256,
        cols=256,
        series_desc="Localizer Scout",
        phantom_type="cylinder"
    )
    ds_scout.save_as(os.path.join(scout_dir, "scout.dcm"), write_like_original=False)
    
    # 3.2 Soft tissue (10 slices)
    st_dir = os.path.join(multi_study_dir, "series_2_soft_tissue")
    os.makedirs(st_dir, exist_ok=True)
    for i in range(1, 11):
        ds = create_ct_dataset(
            study_uid=st_multi,
            series_uid=f"{st_multi}.2",
            instance_num=i,
            z_pos=i * 2.5,
            rows=128,
            cols=128,
            series_desc="Axial Soft Tissue",
            wc=40,
            ww=400,
            phantom_type="cylinder"
        )
        ds.save_as(os.path.join(st_dir, f"axial_{i:02d}.dcm"), write_like_original=False)
        
    # 3.3 Bone window (10 slices)
    bone_dir = os.path.join(multi_study_dir, "series_3_bone")
    os.makedirs(bone_dir, exist_ok=True)
    for i in range(1, 11):
        ds = create_ct_dataset(
            study_uid=st_multi,
            series_uid=f"{st_multi}.3",
            instance_num=i,
            z_pos=i * 2.5,
            rows=128,
            cols=128,
            series_desc="Axial Bone",
            wc=400,
            ww=1800,
            phantom_type="cylinder"
        )
        ds.save_as(os.path.join(bone_dir, f"bone_{i:02d}.dcm"), write_like_original=False)
    print("  [x] Multi-series study created (3 series).")

    # 4. Multi-study directory (Two distinct patients)
    two_pat_dir = os.path.join(fixtures_dir, "dicom", "multi_study", "two_patients_exam")
    os.makedirs(two_pat_dir, exist_ok=True)
    pat_a_dir = os.path.join(two_pat_dir, "patient_A")
    pat_b_dir = os.path.join(two_pat_dir, "patient_B")
    os.makedirs(pat_a_dir, exist_ok=True)
    os.makedirs(pat_b_dir, exist_ok=True)
    
    st_a = f"{UID_ROOT}.4.1"
    for i in range(1, 6):
        ds = create_ct_dataset(
            study_uid=st_a,
            series_uid=f"{st_a}.1",
            instance_num=i,
            z_pos=i * 2.0,
            rows=128,
            cols=128,
            patient_name="PATIENT^ALICE",
            patient_id="E2E_P101",
            study_desc="Exam Alice CT"
        )
        ds.save_as(os.path.join(pat_a_dir, f"alice_{i:02d}.dcm"), write_like_original=False)
        
    st_b = f"{UID_ROOT}.4.2"
    for i in range(1, 6):
        ds = create_ct_dataset(
            study_uid=st_b,
            series_uid=f"{st_b}.1",
            instance_num=i,
            z_pos=i * 2.0,
            rows=128,
            cols=128,
            patient_name="PATIENT^BOB",
            patient_id="E2E_P102",
            study_desc="Exam Bob CT"
        )
        ds.save_as(os.path.join(pat_b_dir, f"bob_{i:02d}.dcm"), write_like_original=False)
    print("  [x] Multi-patient exam created (2 patients).")

    # 5. Edge cases
    edge_dir = os.path.join(fixtures_dir, "dicom", "edge_cases")
    os.makedirs(edge_dir, exist_ok=True)
    
    # 5.1 Empty file
    open(os.path.join(edge_dir, "empty_file.dcm"), "wb").close()
    
    # 5.2 Corrupt magic
    with open(os.path.join(edge_dir, "corrupt_magic.dcm"), "wb") as f:
        f.write(b"\0" * 128 + b"NOPE" + b"\0" * 100)
        
    # 5.3 Truncated pixel body
    ds_trunc = create_ct_dataset(f"{UID_ROOT}.5.1", f"{UID_ROOT}.5.1.1", 1, 0, rows=256, cols=256)
    ds_trunc.PixelData = ds_trunc.PixelData[:512]  # Truncate to 512 bytes
    ds_trunc.save_as(os.path.join(edge_dir, "truncated_pixel.dcm"), write_like_original=False)
    
    # 5.4 Missing UIDs
    ds_nouid = create_ct_dataset(f"{UID_ROOT}.5.2", f"{UID_ROOT}.5.2.1", 1, 0, rows=64, cols=64)
    del ds_nouid.StudyInstanceUID
    del ds_nouid.SeriesInstanceUID
    ds_nouid.save_as(os.path.join(edge_dir, "missing_uids.dcm"), write_like_original=False)
    
    # 5.5 Zero dimensions
    ds_zero = create_ct_dataset(f"{UID_ROOT}.5.3", f"{UID_ROOT}.5.3.1", 1, 0, rows=64, cols=64)
    ds_zero.Rows = 0
    ds_zero.Columns = 0
    ds_zero.PixelData = b""
    ds_zero.save_as(os.path.join(edge_dir, "zero_dimensions.dcm"), write_like_original=False)
    
    # 5.6 Single pixel
    ds_one = create_ct_dataset(f"{UID_ROOT}.5.4", f"{UID_ROOT}.5.4.1", 1, 0, rows=1, cols=1)
    ds_one.PixelData = np.array([[1024]], dtype=np.int16).tobytes()
    ds_one.save_as(os.path.join(edge_dir, "single_pixel.dcm"), write_like_original=False)
    
    # 5.7 Extreme WL
    ds_extwl = create_ct_dataset(f"{UID_ROOT}.5.5", f"{UID_ROOT}.5.5.1", 1, 0, rows=64, cols=64, wc=-9999, ww=0)
    ds_extwl.save_as(os.path.join(edge_dir, "extreme_wl.dcm"), write_like_original=False)
    
    # 5.8 Not a DICOM
    with open(os.path.join(edge_dir, "not_a_dicom.dcm"), "w", encoding="utf-8") as f:
        f.write("This is a plain text file pretending to be DICOM format.\n")
        
    # 5.9 DICOM without extension
    shutil.copy2(os.path.join(single_dir, "ct_axial_512.dcm"), os.path.join(edge_dir, "dicom_no_ext"))
    print("  [x] Edge case datasets created.")

    # 6. ZIP Archives
    arch_dir = os.path.join(fixtures_dir, "archives")
    os.makedirs(arch_dir, exist_ok=True)
    
    # 6.1 Flat ZIP containing 10 slices
    with zipfile.ZipFile(os.path.join(arch_dir, "valid_flat.zip"), "w", zipfile.ZIP_DEFLATED) as zf:
        slices = sorted(os.listdir(head_dir))[:10]
        for s in slices:
            zf.write(os.path.join(head_dir, s), arcname=s)
            
    # 6.2 Nested ZIP
    with zipfile.ZipFile(os.path.join(arch_dir, "valid_nested.zip"), "w", zipfile.ZIP_DEFLATED) as zf:
        for root, _, files in os.walk(st_dir):
            for file in files:
                full_p = os.path.join(root, file)
                rel_p = os.path.join("ExamStudy", "SoftTissue", file)
                zf.write(full_p, arcname=rel_p)
                
    # 6.3 Multi-series ZIP
    with zipfile.ZipFile(os.path.join(arch_dir, "multi_series.zip"), "w", zipfile.ZIP_DEFLATED) as zf:
        for root, _, files in os.walk(multi_study_dir):
            for file in files:
                full_p = os.path.join(root, file)
                rel_p = os.path.relpath(full_p, multi_study_dir)
                zf.write(full_p, arcname=rel_p)
                
    # 6.4 Non-DICOM ZIP
    with zipfile.ZipFile(os.path.join(arch_dir, "non_dicom.zip"), "w", zipfile.ZIP_DEFLATED) as zf:
        zf.writestr("readme.txt", "Sample text documentation.")
        zf.writestr("report.csv", "id,measurement\n1,42.5\n")
        
    # 6.5 Mixed ZIP
    with zipfile.ZipFile(os.path.join(arch_dir, "mixed.zip"), "w", zipfile.ZIP_DEFLATED) as zf:
        zf.write(os.path.join(single_dir, "ct_axial_512.dcm"), arcname="image.dcm")
        zf.writestr("notes.txt", "Radiologist clinical notes.")
        
    # 6.6 Empty ZIP (0-byte)
    open(os.path.join(arch_dir, "empty.zip"), "wb").close()
    
    # 6.7 Corrupted ZIP
    with open(os.path.join(arch_dir, "corrupted.zip"), "wb") as f:
        f.write(b"PK\x03\x04" + b"\x00" * 50)
        
    print("  [x] ZIP archive fixtures created.")
    print("All fixtures generated successfully!")

if __name__ == "__main__":
    generate_all_fixtures()
