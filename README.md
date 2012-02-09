Need to matches on all offsets. Not skip
    Required because of patterns that can overlap
        Trivial case is a blank file
            Pattern is blank
            Text is blank
            => Matches everywhere
            
BM matching speed is fast if pattern is a small fraction of length of text.
    There is an overhead of creating skip tables

num_pages, num_copies, copy_size (bytes), total_size (MB) duration (sec) speed (MB/sec)
    4,    2,       40,  0.0,   0.00, 62.653
   20,    2,      900,  0.0,   0.00, 114.773
   40,   20,    50000,  1.0,   0.02, 45.296
  400,  200,    50000,  9.5,   0.03, 319.212
  400,  200,   500000, 95.4,   2.07, 46.106
 4000, 2000,    50000, 95.4,   0.12, 789.226
400000, 200000,      500, 95.4,   0.15, 626.208
400000, 200000,     5000, 953.7,   1.08, 882.073
   40,   20,   500000,  9.5,   1.94,  4.923
   34,   17,   500000,  8.1,   1.96,  4.128
   51,   17,   500001,  8.1,   1.95,  4.155
   68,   17,   500000,  8.1,   1.95,  4.165
           

Scan speed ranges from 4MB/sec to 900 MB/sec depending on the pattern/text size ratio

           