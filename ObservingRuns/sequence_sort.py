import manage as man
def WriteFileCols(values,fname):
    with open(fname,"w") as data:
        for i in range(len(values)):
            data.write("{0} {1}\n".format(values[i][0],values[i][1]))
n=33
while n < 49:
    try:
        fname='node{0}/sequence.all.0329_june29.dat'.format(n)
        sequence=man.LoadData(fname)
        time=man.IterativeIntAppend(sequence,0)
        disk=man.IterativeIntAppend(sequence,1)
        sequence=[]
        for i in range(len(time)):
            point=[time[i],disk[i]]
            sequence.append(point)
        sequence.sort()
        name='node{0}/sequence.sorted.dat'.format(n)
        WriteFileCols(sequence,name)
        n+=1
    except IOError:
        print 'No sequence file on node{0}'.format(n)
        n+=1
        pass
        