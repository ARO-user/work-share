import manage as man

n=33
while n < 49:
    try:
        fname='node{0}/generated_sequence.dat'.format(n)
        gen_sequence=man.LoadData(fname)
        gen_stamp=man.IterativeIntAppend(gen_sequence,0)
        fname='node{0}/sequence.sorted.dat'.format(n)
        sequence=man.LoadData(fname)
        stamp=man.IterativeIntAppend(sequence,0)
        if len(gen_stamp)!=len(stamp):
            diff=len(stamp)-len(gen_stamp)
            print '''Diffrent number of timestamps for node{0}\n
            {1} in generated file\n
            {2} in actual file\n
            {3} difference\n'''.format(n,len(gen_stamp),len(stamp),diff)
        else:
            for i in range(len(stamp)):
                point=gen_stamp[i]-stamp[i]
                if point!=0:
                    print 'Mismatched timestamp number, entry number {0}'.format(i+1)
        n+=1
    except IOError:
        print 'Missing sequence file on node{0}'.format(n)
        n+=1
        pass
   
    
