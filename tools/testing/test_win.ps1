Set-StrictMode -Version Latest
$ErrorActionPreference = "SilentlyContinue"
$PrintOutput = "True"
$StopOnFirstTestFail = "False"

$global:CompoundOutput = 0

function compoundReturnValue( $e )
{
  if( $e -ne 0)
  {
    $global:CompoundOutput = $global:CompoundOutput + 1;
  }
  # Write-Host "compound output: $global:CompoundOutput";
}

$CURRENT_PATH = pwd
cd $PSScriptRoot/../..

Write-Host "" ""

Get-ChildItem -Filter bin/test_*.exe | ForEach {
  if ( $PrintOutput -eq "True" ) {
    &$_.FullName;
  } else {
    Write-Host " $_  => " -NoNewline
    # ($out = &$_.Fullname) ;
    ($out = &$_.Fullname) 2>&1 | out-null;
    # &$_.Fullname 2>&1 | Out-Null;
  }
  compoundReturnValue $lastexitcode;
  if ( $lastexitcode -ne 0 ) { 
    Write-Host " FAIL!"  -ForegroundColor Red;
    if ($StopOnFirstTestFail -eq "True" ) {
      exit $lastexitcode
    }
  }  else { 
    Write-Host " RockSolid!" -ForegroundColor Green;
  }
}

cd $CURRENT_PATH

exit $CompoundOutput;

