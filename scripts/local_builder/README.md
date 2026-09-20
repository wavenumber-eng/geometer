# Local WSL2 builder

This builder is the Ubuntu 22.04 x64 qualification environment for Linux x64
and WASM release candidates. It deliberately has no Windows-drive mounts,
Windows executable interoperability, sudo-capable build user, or publication
credentials.

Create it on a Windows data volume:

```powershell
wsl.exe --install Ubuntu-22.04 `
  --name Geometer-Ubuntu-22.04 `
  --location D:\WSL\Geometer-Ubuntu-22.04 `
  --no-launch
wsl.exe -d Geometer-Ubuntu-22.04 -u root -- `
  bash /mnt/c/eli/wn-hw/geometer/scripts/provision_local_builder_wsl.sh
wsl.exe --terminate Geometer-Ubuntu-22.04
```

Provisioning pins and verifies the downloaded uv, CMake, Chrome-for-Testing,
Node, npm, and Rust toolchain inputs. Ubuntu packages come from the Ubuntu
22.04 security/update repositories; record their installed versions in each
candidate's builder evidence rather than inventing another dependency-key
scheme.

After restart, `geometer-builder` is the default user. Put the exact reviewed
source revision under `/work/geometer/` using the candidate workflow checkout
or a one-time local Git bundle. Do not re-enable Windows mounts or interop for
normal builds.

This environment produces untrusted candidate bytes only. It must not contain
PyPI tokens, R2 write credentials, GitHub release credentials, a developer
`.env`, SSH agents, or cloud CLI sessions. Publication remains a separate
hosted operation.
