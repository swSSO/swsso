$dllpath=join-path $(get-location) "swSSOPwdProtect.dll"
[void][Reflection.Assembly]::LoadFile($dllpath)

$encrypted=[swSSOPwdProtect]::Encrypt($args)
Write-Host ("Encrypted: {0}`r`n" -f $encrypted)

$decrypted = [swSSOPwdProtect]::Decrypt($encrypted)
Write-Host ("Decrypted: {0}`r`n" -f $decrypted)
