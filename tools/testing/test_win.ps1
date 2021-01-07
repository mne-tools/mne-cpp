$CURRENT_PATH = pwd
cd $PSScriptRoot/../..
Get-ChildItem -Filter bin/test_*.exe | ForEach {Write-Output "" "" Starting $_.Fullname; &$_.Fullname}
cd $CURRENT_PATH
