# Requires VSSetup https://github.com/microsoft/vssetup.powershell

#Install VC 14.39
$vs_installation_path = $(Get-VSSetupInstance -All | Select-VSSetupInstance -Version '[17.0,)').InstallationPath
$process = Start-Process -FilePath "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vs_installer.exe" -ArgumentList "modify --installPath `"$vs_installation_path`" --quiet --norestart --nocache --add Microsoft.VisualStudio.Component.VC.14.39.17.9.x86.x64" -PassThru
$process.WaitForExit();

# Workaround for a MSVC issue that may get fixed in the future https://developercommunity.visualstudio.com/t/MicrosoftVCToolsVersion1436176prop/10385615
$props_file = Join-Path -Path $vs_installation_path -ChildPath "VC\Auxiliary\Build\14.39.17.9\Microsoft.VCToolsVersion.14.39.17.9.props"
$data = Get-Content $props_file
$broken_redist_string = '<Import Project="$([System.IO.Path]::GetFullPath($(MSBuildThisFileDirectory)Microsoft.VCRedistVersion.default.props))"/>'
$fixed_redist_string = '<Import Project="$([System.IO.Path]::GetFullPath($(MSBuildThisFileDirectory)..\Microsoft.VCRedistVersion.default.props))"/>'
$new_data = $data.Replace($broken_redist_string, $fixed_redist_string)
$new_data | Out-File -Path $props_file
