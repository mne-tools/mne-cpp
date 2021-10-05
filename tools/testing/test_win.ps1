Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$CURRENT_PATH = pwd

cd $PSScriptRoot/../..

Get-ChildItem -Filter bin/test_*.exe | ForEach {
  Write-Output "" "" "Starting $_" ""; 
  &$_.Fullname; 
  if (($lastexitcode -ne 0) -and $ErrorActionPreference -eq "Stop") { 
    Write-Output "" "" "$_ IS FAILING" "" "";
    exit $lastexitcode
  }  else { 
    Write-Output "" "" "$_ FINISHED OK!" "" "";
  }
}

# Write-Host "$output"
cd $CURRENT_PATH
