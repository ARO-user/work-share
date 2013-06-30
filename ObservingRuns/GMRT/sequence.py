import manage as man
from astropy.time import Time
def WriteFile(values,fname):
    with open(fname,"w") as data:
        for i in range(len(values)):
            data.write("{0}\t{1}\n".format(values[i][1],values[i][0]))

n=33
while n<49:
    mastertime=[]
    i=1
    while i<5:
        fname='node{0}/timestamp_voltage.all.0329_june29.{1}.dat'.format(n,i)
        try:
            times=man.LoadData(fname)
            year=man.IterativeFloatAppend(times,0)
            month=man.IterativeStrAppend(times,1)
            day=man.IterativeStrAppend(times,2)
            hour=man.IterativeStrAppend(times,3)
            minute=man.IterativeStrAppend(times,4)
            seconds=[]
            for j in range(len(times)):
                a=man.IterativeIntAppend(times,5)
                b=man.IterativeFloatAppend(times,6)
                point=a[j]+b[j]
                seconds.append(point)

                time=[]
            for j in range(len(times)):
                point="{0}-{1}-{2} {3}:{4}:{5}".format(year[j],month[j],day[j],hour[j],minute[j],seconds[j])
                time.append(point)
            t=Time(time,format='iso',scale='utc')
            mjd=t.mjd
            for j in range(len(mjd)):
                point=[mjd[j],i-1]
                mastertime.append(point)
            print "Done node{0}/timestamp_voltage.all.0329_june29.{1}.dat".format(n,i)
            i+=1 
        except IOError:
            print "Missing node{0}/timestamp_voltage.all.0329_june29.{1}.dat".format(n,i)
            i+=1
            pass
        
    mastertime.sort()   

    name='node{0}/SequencedTimeStamp.dat'.format(n)
    WriteFile(mastertime,name)
    print 'Done sequenced time stamp node{0}'.format(n)
    n+=1
    
