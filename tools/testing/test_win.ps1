Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$PrintOutput = "False"
$StopOnFirstTestFail = "False"

$global:CompoundOutput = 0

function compoundReturnValue( $e )
{
  if( $e -ne 0)
  {
    $global:CompoundOutput = $global:CompoundOutput + 1;
  }
  Write-Output "compound output: $global:CompoundOutput";
}

$CURRENT_PATH = pwd
cd $PSScriptRoot/../..

Get-ChildItem -Filter bin/test_*.exe | ForEach {
  Write-Output "" "" "Starting $_" "";
  if ( $PrintOutput -eq "True" ) {
    &$_.FullName;
  } else {
    $out = &$_.Fullname;
    # &dir ;
  }
  compoundReturnValue $lastexitcode;
  if (($lastexitcode -ne 0) -and $ErrorActionPreference -eq "Stop") { 
    Write-Output " => IS FAILING" "";
    if ($StopOnFirstTestFail -eq "True" ) {
      exit $lastexitcode
    }
  }  else { 
    Write-Output " => FINISHED OK!" "";
  }
}

cd $CURRENT_PATH

exit $CompoundOutput;

