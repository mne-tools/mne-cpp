Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$CURRENT_PATH = pwd

cd $PSScriptRoot/../..

Get-ChildItem -Filter bin/test_*.exe | ForEach {
  Write-Output "" "" Starting $_.Fullname; 
  &$_.Fullname; if (($lastexitcode -ne 0) -and $ErrorActionPreference -eq "Stop") { 
    exit $lastexitcode
  }  
}

# Write-Host "$output"
cd $CURRENT_PATH
