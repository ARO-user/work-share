import astropy.units as u
from astropy.time import Time
import manage as man
import observability
import numpy as np

def WriteFileCols(values1,values2,fname):
    with open(fname,"w") as data:
        for i in range(len(values1)):
            data.write("{0}\t{1}\n".format(values1[i],values2[i]))

#File is formatted with spaces in between year, month, day etc. For some reason,# the seconds and anything less than seconds are specified separately

fname="timestamp_voltage.b1957+20.raw0.node0.scan0"
times=man.LoadData(fname)
year=man.IterativeStrAppend(times,0)
month=man.IterativeStrAppend(times,1)
day=man.IterativeStrAppend(times,2)
hour=man.IterativeStrAppend(times,3)
minute=man.IterativeStrAppend(times,4)

seconds=[]
for i in range(len(times)-2):
    a=man.IterativeIntAppend(times,5)
    b=man.IterativeFloatAppend(times,6)
    point=a[i]+b[i]
    seconds.append(point)

time=[]
for i in range(len(times)-2):
    point="{0}-{1}-{2} {3}:{4}:{5}".format(year[i],month[i],day[i],hour[i],minute[i],seconds[i])
    time.append(point)

t=Time(time, scale='utc')
mjd=t.mjd

tjd=mjd-40000 #Convert modified Julian Date to truncated Julian Date

#Create an astropy Time object that includes the coordinates of GMRT
mjd = Time(mjd, format='mjd', scale='utc', 
               lon=(74*u.deg+02*u.arcmin+59.07*u.arcsec).to(u.deg).value,
               lat=(19*u.deg+05*u.arcmin+47.46*u.arcsec).to(u.deg).value)


#Greenwich mean sidereal time (in hours)
gmst=observability.time2gmst(mjd)
rad_gmst=gmst*np.pi/12

WriteFileCols(tjd,rad_gmst,"mod_{0}.dat".format(fname))
