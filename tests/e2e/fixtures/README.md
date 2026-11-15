# AlizaMS E2E Test Fixtures Catalog

This catalog documents the synthetic DICOM fixtures and archive files used by the AlizaMS automated E2E test harness.

## Directory Structure
- `dicom/single_slice/`
  - `ct_axial_512.dcm`: Calibrated 512x512 CT axial slice (0.5mm pixel spacing, 50mm disc & 20x20mm square phantom).
  - `mr_brain_256.dcm`: 256x256 MR axial slice with smooth intensity gradient and anatomical phantom.
  - `us_cardiac.dcm`: Ultrasound 600x800 single frame (TESTXX02).
  - `sr_report.dcm`: Structured Report DICOM dataset (TESTXX04_SR).
- `dicom/multi_slice_series/`
  - `ct_head_30slices/`: 30 slices (512x512, 2.0mm thickness, 3D spherical phantom for MPR and scrolling).
  - `ct_thorax_100slices/`: 100 slices (512x512, 1.0mm thickness for high-frequency scroll benchmarks).
- `dicom/multi_series_study/`
  - `study_multiseries/`: Multi-series study with single StudyInstanceUID and 3 series:
    - `series_1_scout`: Scout localizer slice.
    - `series_2_soft_tissue`: 10 slices soft tissue window.
    - `series_3_bone`: 10 slices bone window.
- `dicom/multi_study/`
  - `two_patients_exam/`: Shared folder containing exams for two distinct patients:
    - Patient A (`E2E_PATIENT_A`, StudyUID: `1.2.826.0.1.3680043.8.498.1001`)
    - Patient B (`E2E_PATIENT_B`, StudyUID: `1.2.826.0.1.3680043.8.498.1002`)
- `dicom/edge_cases/`
  - `empty_file.dcm`: 0-byte file.
  - `corrupt_magic.dcm`: Corrupted preamble with `NOPE` instead of `DICM`.
  - `truncated_pixel.dcm`: Header declaring 512x512 pixels but payload truncated to 1KB.
  - `missing_uids.dcm`: Valid DICOM dataset lacking StudyInstanceUID and SeriesInstanceUID tags.
  - `zero_dimensions.dcm`: Rows=0, Columns=0.
  - `single_pixel.dcm`: Rows=1, Columns=1.
  - `extreme_wl.dcm`: WindowCenter=-9999, WindowWidth=0.
  - `not_a_dicom.dcm`: Plain ASCII text file named `.dcm`.
  - `dicom_no_ext`: Valid DICOM file without file extension.
- `archives/`
  - `valid_flat.zip`: Flat ZIP containing DICOM files directly at root.
  - `valid_nested.zip`: Hierarchical ZIP containing `study/series/*.dcm`.
  - `multi_series.zip`: ZIP containing multiple DICOM series.
  - `non_dicom.zip`: ZIP containing non-DICOM assets (TXT, PNG).
  - `mixed.zip`: ZIP containing both DICOM slices and non-DICOM text files.
  - `empty.zip`: 0-byte ZIP file.
  - `corrupted.zip`: Invalid/truncated ZIP header.

## Generation
All fixtures (except imported samples) are generated deterministically by `tests/e2e/scripts/generate_fixtures.py`.
