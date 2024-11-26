# System Programming Lab 11 Multiprocessing

#Overview
   ___
    |  HE MEANS BY WHICH I had implemented the movie frame generation was not a complex one,
(though it came with its complications) and is relatively effective. It's as simple as splitting up the 
50 frames into a given number of child processes. If it cannot evenly split the amount of image computations
by 50, it leaves all leftovers to the final child process.

#Results
   /\  /\
  /  \/  \ Y IMPLEMENTATION OF THIS PROGRAM, has a time complexity of approximately O(n^-0.6), where n is the number
of child processes used, as shown in the included graph.