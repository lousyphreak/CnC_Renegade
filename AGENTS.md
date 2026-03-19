# Command and Conquer: Renegade

This is the source code for Command and Conquer: Renegade, and our goal is to port it to a modern tech stack, while preserving the original gameplay and experience as much as possible.

there is an initial codebase analysis in `docs/investigation`, use it as a reference, but **ALWAYS** refer to the source code itself as the ultimate source of truth. The codebase is not well documented, and there are many mysteries to solve, so be prepared to do a lot of reading and experimentation.

No "case forwarding headers": if an include directive has the wrong case, fix it.

Use SDL3 for anything that it provides, to make sure the game runs on as many platforms as possible.
