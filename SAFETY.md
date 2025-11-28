# Cppcheck Safety Notes

## Process Stability

By default the analysis is performed in the main process. So if you encounter an assertion or crash bug within Cppcheck the analysis will abort prematurely.

If you use `-j<n>` a new process (up to the amount of `n`) will be spawned for each file which is analyzed (currently only available on non-Windows platforms - see https://trac.cppcheck.net/ticket/12464). This will make sure the analysis will always finish and a result is being provided. Assertion and crash fixes will be provided as `cppcheckError` findings. If you do not want to spawn process you can use `--executor=thread` to use threads instead.

If you encountered any such bugs please report them at https://trac.cppcheck.net/.

### Note On Handling Unknown/Untrusted Code

The internal parsing of Cppcheck is very lenient and is able to handle code which will not be compilable. This is done so you can analyze code without providing all dependencies.

But this also means that code which is is invalid will not be rejected outright. This is the most common cause of abnormal process terminations and is most likely caused by unknown/untrusted code (provided by e.g. a pull request). To reduce the chance of this happening you should not invoke the analysis *after* that code has been successfully compiled. This might also limit someone leveraging a known cause for the abnormal process termination for malicious intents.

## Code Execution

The analyzed code will only be parsed and/or compiled but no executable code is being generated. So there is no risk in analyzing code containing known exploits as it will never be executed.

## Process Elevation

No process elevation is being performed for any of the spawned processes. They currently use the same permissions as the main process.

See https://trac.cppcheck.net/ticket/14237 about a potential future improvement by reducing the permissions of spawned processes.

## Invoked executables

By default no additional external executables will be invoked by the analysis.

But there are some options which will utilize additional executables.

`python`/`python3` - Used for the execution of addons. Trigger by using the `--addon=<addon>` option and specifying addons in `cppcheck.cfg`.
The executable will be looked for in `PATH` by default and can be configured by TODO.

`clang-tidy` - Used for invoking an additional analysis via Clang-Tidy. Triggered by the `--clang-tidy` option.
The executable will be looked for in `PATH` by default. A versioned executable or absolute path can be specified by using `--clang-tidy=<exe>`.

`clang` - Used to generate a CLang AST instead of an internal one. Triggered by the `--clang` option.
The executable will be looked for in `PATH` by default. A versioned executable or absolute path can be specified by using `--clang=<exe>`.

## Output Files

By default no files will be written.

But there are some options which will cause files to be written.

`--cppcheck-build-dir=<dir>` - will create files inside the specified directory only - the specified directory needs to be created by the user before running the analysis
`--dump` - when `--cppcheck-build-dir` is *not* specified `<file>.dump` files will be created next to files which are being analyzed (TODO: `--addon`)
`--plist-output=<dir>` - will create files inside the specified directory only - the specified directory needs to be created by the user before running the analysis
TODO: *.ctu-info
TODO: document usage of temporary folder

## Required Permissions

By default you only need the permissions to read the files specified for analysis and the specified include paths. No administrator permissions should be necessary to perform the analysis.