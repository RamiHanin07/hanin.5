# hanin.5
CS4750 Paging Simulator

This project went rather well for me I think, I had a really clear goal and vision for how the project would work,
and was able to implement my ideas rather effectively in a quick manner.

I didn't have a ton of problems along the way when it came to implementation, my biggest problem was just bug fixing.

I was unable to get GDB working, as I kept getting a "debugging symbols missing" error, so it was really hard to debug logic, where
if I had GDB available to me, it would have made logic errors much quicker. A lot of times, I would just accidentally use assignment = in if statements, instead of == , and it would ruin everything until I finally figured out the error with spastic cout statements.

Otherwise, I think it went rather well.

To my knowledge, no outstanding problems.

Actually, there is one. I'm having an issue, which I bandaid patched, where sometimes my child processes will read an address in the 5000 range,
and this will set my clock->sec equal to that read address in the 5000 range. I have absolutely no idea how this is happening.
There is no point in which the child processes are ever interacting with the clock->sec directly, there is no assignment operators used in the program for clock->sec, I only ever use clock->sec++ to increment by one. I have absolutely no idea how this happening, or why this is happening, and it is far and away the most frustrating thing that I dealt with this project. I assume its a memory error, where sometimes the child processes are accessing the clock memory address unintentionally, but I have no idea how thats happening. it's currently bandaid fixed, but if I were to release this as a product, it would be with the memory error fixed.

Thank you!