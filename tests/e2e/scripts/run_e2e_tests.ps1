# run_e2e_tests.ps1 — Master E2E Test Runner for AlizaMS
param(
    [ValidateSet("1", "2", "3", "4", "All")]
    [string]$Tier = "All",
    
    [string]$BinaryPath = "",
    
    [switch]$GenerateFixtures = $false
)

$ErrorActionPreference = "Stop"

if ($BinaryPath -and (-not (Test-Path -Path $BinaryPath -PathType Leaf))) {
    Write-Error "Specified BinaryPath does not exist: $BinaryPath"
    exit 1
}

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$BaseTestsDir = Split-Path -Parent $ScriptDir
$FixturesDir = Join-Path $BaseTestsDir "fixtures"
$ReportsDir = Join-Path $BaseTestsDir "reports"
$CasesDir = Join-Path $ScriptDir "cases"

# Ensure reports directory exists
if (-not (Test-Path $ReportsDir)) {
    New-Item -ItemType Directory -Path $ReportsDir -Force | Out-Null
}

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " AlizaMS E2E Master Test Runner" -ForegroundColor Cyan
Write-Host " Target Tier: $Tier" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

# 1. Generate fixtures if needed
$SingleSliceDcm = Join-Path $FixturesDir "dicom\single_slice\ct_axial_512.dcm"
if ($GenerateFixtures -or (-not (Test-Path $SingleSliceDcm))) {
    Write-Host "`n[Step 1] Generating and verifying fixtures..." -ForegroundColor Yellow
    $GenScript = Join-Path $ScriptDir "generate_fixtures.py"
    python $GenScript
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Fixture generation failed with exit code $LASTEXITCODE"
        exit $LASTEXITCODE
    }
    
    $VerifyScript = Join-Path $ScriptDir "verify_fixtures.py"
    python $VerifyScript
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Fixture verification failed with exit code $LASTEXITCODE"
        exit $LASTEXITCODE
    }
}

# 2. Python runner script wrapper to aggregate results
$AggregatorScript = @"
import os
import sys
import json
import time

cases_dir = r'$CasesDir'
sys.path.insert(0, cases_dir)

import test_tier1_coverage
import test_tier2_corners
import test_tier3_pairwise
import test_tier4_workflows

tier = '$Tier'
bin_path = r'$BinaryPath' if r'$BinaryPath' else None

overall_summary = {
    'timestamp': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
    'tier_requested': tier,
    'total_tests': 0,
    'total_passed': 0,
    'total_failed': 0,
    'total_skipped': 0,
    'pass_rate': 0.0,
    'total_duration_sec': 0.0,
    'tiers': []
}

t_start = time.time()

if tier in ['1', 'All']:
    s1 = test_tier1_coverage.run_all_tier1(bin_path)
    overall_summary['tiers'].append(s1)

if tier in ['2', 'All']:
    s2 = test_tier2_corners.run_all_tier2(bin_path)
    overall_summary['tiers'].append(s2)

if tier in ['3', 'All']:
    s3 = test_tier3_pairwise.run_all_tier3(bin_path)
    overall_summary['tiers'].append(s3)

if tier in ['4', 'All']:
    s4 = test_tier4_workflows.run_all_tier4(bin_path)
    overall_summary['tiers'].append(s4)

overall_summary['total_duration_sec'] = time.time() - t_start
for t in overall_summary['tiers']:
    overall_summary['total_tests'] += t['total']
    overall_summary['total_passed'] += t['passed']
    overall_summary['total_failed'] += t['failed']
    overall_summary['total_skipped'] += t['skipped']

if overall_summary['total_tests'] > 0:
    overall_summary['pass_rate'] = (overall_summary['total_passed'] / overall_summary['total_tests']) * 100.0

reports_dir = r'$ReportsDir'
summary_path = os.path.join(reports_dir, 'summary.json')
with open(summary_path, 'w', encoding='utf-8') as f:
    json.dump(overall_summary, f, indent=2)

print(f'\nRecorded JSON summary to {summary_path}')
sys.exit(0 if overall_summary['total_failed'] == 0 else 1)
"@

$TempDir = [System.IO.Path]::GetTempPath()
$TempAggregator = Join-Path $TempDir ("e2e_aggregator_" + [System.Guid]::NewGuid().ToString("N") + ".py")

try {
    Set-Content -Path $TempAggregator -Value $AggregatorScript -Encoding UTF8
    Write-Host "`n[Step 2] Executing test suite..." -ForegroundColor Yellow
    python -u $TempAggregator
    $RunExitCode = $LASTEXITCODE
} finally {
    if (Test-Path $TempAggregator) {
        Remove-Item -Path $TempAggregator -Force -ErrorAction SilentlyContinue
    }
}

Write-Host "`n==========================================================" -ForegroundColor Cyan
if ($RunExitCode -eq 0) {
    Write-Host " ALL E2E TESTS PASSED SUCCESSFULLY! (Exit Code: 0)" -ForegroundColor Green
} else {
    Write-Host " E2E TEST RUN COMPLETED WITH FAILURES (Exit Code: $RunExitCode)" -ForegroundColor Red
}
Write-Host "==========================================================" -ForegroundColor Cyan

exit $RunExitCode
