$dllpath=join-path $(get-location) "swSSOPwdProtect.dll"

[void][Reflection.Assembly]::LoadFile($dllpath)

$encrypted=[swSSOPwdProtect]::Encrypt($args)
$encrypted | Out-File password.txt
