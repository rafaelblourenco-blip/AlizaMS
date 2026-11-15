#!/usr/bin/env python3
"""
verify_fixtures.py — Verify integrity and readability of all generated E2E fixtures.
"""

import os
import sys
import zipfile
import pydicom

def verify_all():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    fixtures_dir = os.path.abspath(os.path.join(script_dir, "..", "fixtures"))
    
    print(f"Verifying fixtures in {fixtures_dir}...")
    errors = []
    
    # 1. Single slice
    s1 = os.path.join(fixtures_dir, "dicom", "single_slice", "ct_axial_512.dcm")
    if not os.path.isfile(s1):
        errors.append(f"Missing {s1}")
    else:
        try:
            ds = pydicom.dcmread(s1)
            assert ds.Rows == 512 and ds.Columns == 512, "Invalid rows/cols"
        except Exception as e:
            errors.append(f"Failed to read {s1}: {e}")
            
    # 2. Multi-slice series
    h30 = os.path.join(fixtures_dir, "dicom", "multi_slice_series", "ct_head_30slices")
    if not os.path.isdir(h30):
        errors.append(f"Missing directory: {h30}")
    else:
        files_30 = [f for f in os.listdir(h30) if f.endswith(".dcm")]
        if len(files_30) != 30:
            errors.append(f"Expected 30 slices in ct_head_30slices, found {len(files_30)}")
        
    t100 = os.path.join(fixtures_dir, "dicom", "multi_slice_series", "ct_thorax_100slices")
    if not os.path.isdir(t100):
        errors.append(f"Missing directory: {t100}")
    else:
        files_100 = [f for f in os.listdir(t100) if f.endswith(".dcm")]
        if len(files_100) != 100:
            errors.append(f"Expected 100 slices in ct_thorax_100slices, found {len(files_100)}")
        
    # 3. Archives
    zf_flat = os.path.join(fixtures_dir, "archives", "valid_flat.zip")
    if not zipfile.is_zipfile(zf_flat):
        errors.append(f"Invalid zip: {zf_flat}")
        
    zf_nested = os.path.join(fixtures_dir, "archives", "valid_nested.zip")
    if not zipfile.is_zipfile(zf_nested):
        errors.append(f"Invalid zip: {zf_nested}")
        
    if errors:
        print("VERIFICATION FAILED:")
        for err in errors:
            print("  -", err)
        return False
        
    print("ALL FIXTURES VERIFIED SUCCESSFULLY!")
    return True

if __name__ == "__main__":
    success = verify_all()
    sys.exit(0 if success else 1)
