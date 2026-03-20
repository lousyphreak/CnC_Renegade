# CMake target order

This file lists the buildable CMake targets for the current Linux Makefiles build in a topological order: if target A depends on target B, B appears first.

Source of truth:
- Top-level `CMakeLists.txt`
- `Code/CMakeLists.txt`
- `Code/*/CMakeLists.txt`
- `external/SDL/CMakeLists.txt`
- `cmake/RenegadeTargets.cmake`
- `cmake --build build --target help`

Notes:
- CMake helper targets such as `all`, `clean`, `depend`, `edit_cache`, and `rebuild_cache` are omitted.
- `SDL3::SDL3` is an alias for the build target `SDL3-shared`.
- Every project target configured through `renegade_configure_target()` also depends on the interface target `renegade::project_options`.
- `renegade::project_options` is interface-only, so it is listed separately and not counted as a buildable target.

## Build order

1. `renegade::project_options` — interface target used by all project targets.
2. `SDL_uclibc`
3. `SDL3-shared` — depends on `SDL_uclibc`.
##4. `wwlib` — depends on `SDL3-shared` via `SDL3::SDL3`.
##5. `wwdebug` — depends on `SDL3-shared` via `SDL3::SDL3`.
##6. `wwutil` — depends on `SDL3-shared` via `SDL3::SDL3`.
##7. `wwbitpack`
##8. `WWMath` — depends on `wwlib`.
##9. `wwsaveload` — depends on `wwlib`, `wwdebug`, `WWMath`, and `wwutil`.
##10. `wwtranslatedb`
11. `BinkMovie` — depends on `wwlib`, `wwdebug`, and `WWMath`.
##12. `Scripts` — depends on `wwlib`, `WWMath`, and `wwdebug`.
13. `wwnet` — depends on `wwlib`, `wwdebug`, `WWMath`, `wwutil`, `wwsaveload`, and `wwbitpack`.
14. `WWAudio` — depends on `wwlib`, `wwdebug`, `WWMath`, `wwutil`, `wwsaveload`, and `wwtranslatedb`.
15. `wwui` — depends on `wwlib`, `wwdebug`, `WWMath`, `wwutil`, `wwsaveload`, and `wwtranslatedb`.
16. `wwphys` — depends on `wwlib`, `wwdebug`, `WWMath`, `wwutil`, `wwsaveload`, `wwbitpack`, and `wwtranslatedb`.
17. `Combat` — depends on `wwlib`, `wwdebug`, `WWMath`, `wwutil`, `wwsaveload`, `wwbitpack`, `wwtranslatedb`, `wwui`, `wwnet`, `WWAudio`, `wwphys`, and `BinkMovie`.
18. `CommandoLib` — depends on `BinkMovie`, `Combat`, `Scripts`, `WWAudio`, `WWMath`, `wwbitpack`, `wwdebug`, `wwnet`, `wwphys`, `wwsaveload`, `wwtranslatedb`, `wwui`, `wwutil`, `wwlib`, and `SDL3-shared`.
19. `CommandoDedicatedLib` — depends on `BinkMovie`, `Combat`, `Scripts`, `WWAudio`, `WWMath`, `wwbitpack`, `wwdebug`, `wwnet`, `wwphys`, `wwsaveload`, `wwtranslatedb`, `wwui`, `wwutil`, `wwlib`, and `SDL3-shared`.
20. `Commando` — depends on `CommandoLib` and `SDL3-shared`.
21. `CommandoDedicated` — depends on `CommandoDedicatedLib` and `SDL3-shared`.

## Buildable targets from `cmake --build build --target help`

- `SDL_uclibc`
- `SDL3-shared`
- `wwlib`
- `wwdebug`
- `wwutil`
- `wwbitpack`
- `WWMath`
- `wwsaveload`
- `wwtranslatedb`
- `BinkMovie`
- `Scripts`
- `wwnet`
- `WWAudio`
- `wwui`
- `wwphys`
- `Combat`
- `CommandoLib`
- `CommandoDedicatedLib`
- `Commando`
- `CommandoDedicated`
