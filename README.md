# CHROMADDED-VOIP-CLIENT-BASE

A native client library for 3D positional voice communication.

CHROMADDED-VOIP-CLIENT-BASE is the client-side component of the CHROMADDED-VOIP project, designed to provide the foundation for real-time voice communication with positional audio in 3D environments.

The project is written in C/C++ and is currently under active development.

Overview

The library is intended to work alongside the CHROMADDED-VOIP server, providing a client-side foundation for applications that require networked voice communication.

Project goals

- 3D positional voice communication — support for voice chat in spatial environments.
- Native implementation — a C/C++ codebase intended for native applications.
- Client-server architecture — designed to operate with the CHROMADDED-VOIP server.
- Cross-platform development — build configurations for Windows and Linux.

«Status: Work in progress. Build requirements, APIs, and supported features may change as development continues.»

Related project

This library is designed to work with the CHROMADDED-VOIP server.

- "CHROMADDED-VOIP — Server repository" (https://github.com/Mauricio-MN/CHROMADDED-VOIP)

Requirements

Windows

- "MSYS2" (https://www.msys2.org/)
- A compatible GCC toolchain
- Any additional dependencies required by the project

Linux

- GNU GCC or a compatible C++ compiler
- GNU Make
- Any additional development libraries required by the project

Building

Clone the repository:

git clone https://github.com/Mauricio-MN/CHROMADDED-VOIP-CLIENT-BASE.git
cd CHROMADDED-VOIP-CLIENT-BASE

Windows

Install MSYS2 and configure a compatible GCC toolchain. Open the appropriate MSYS2 terminal, navigate to the project directory, and use the provided build scripts or Makefile.

Linux

Ensure that GCC and GNU Make are installed, then navigate to the project directory and use the provided Makefile.

Note: Exact build commands and dependency installation steps will be documented as the build process is finalized.

Repository structure

Path| Description
"include/"| Header files and public interfaces
"src/"| Source code
"Makefile"| Build configuration
"Makefile2"| Additional build configuration
"buildall.sh"| Build script
"copydlls.sh"| DLL copy script
"copytest.sh"| Test-related copy script
"dllfinder.sh"| DLL discovery utility

Development status

This project is under development. Interfaces, build configurations, and implementation details may evolve over time.

Contributions, testing, and technical feedback are welcome.

License

See the "LICENSE" (LICENSE) file for licensing terms.

---

Part of the CHROMADDED-VOIP project.
