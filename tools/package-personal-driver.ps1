param([string]$CertificateThumbprint = 'CA50595A5BC03E62BF384B5DC39660D1E9210A9B')
# Old COL04 paths are retired. This builds the reviewed COL01 candidates only.
& (Join-Path $PSScriptRoot 'package-col01-driver.ps1') -Variant Both -CertificateThumbprint $CertificateThumbprint
