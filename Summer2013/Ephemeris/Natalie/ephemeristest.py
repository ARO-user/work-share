import numpy as np
from astropy.time import Time
from astropy.coordinates.angles import Angle
from astropy.constants import c
import astropy.units as u
import de405
import observability
import pulsar
from pulsar import ELL1Ephemeris
import barycentre
from barycentre import JPLEphemeris
from manage import *

eph1957 = ELL1Ephemeris('ForMarten.par')
jpleph = JPLEphemeris(de405)

data=LoadData('ForMartenCleaned')
del data[0]
del data[0]
MJD_TDB=IterativeFloatAppend(data,3)
mjd = Time(MJD_TDB,format='mjd',scale='tt').mjd
mjd = Time(mjd, format='mjd', scale='tt', 
           lon=(02*u.deg+18*u.arcmin+25.7*u.arcsec).to(u.deg).value,
           lat=(53*u.deg+14*u.arcmin+10.5*u.arcsec).to(u.deg).value)
time=mjd.tdb.mjd
time_jd=mjd.tdb.jd
arrival=[]
for i in range(len(time)):
    f_p=eph1957.evaluate('F',time[i],t0par='PEPOCH')
    P_0=1./f_p
    mjd = Time(time[i],format='mjd', scale='tt').mjd
    d_orb = eph1957.orbital_delay(time[i])
    v_orb = eph1957.radial_velocity(time[i])
    time[i]+=d_orb/(3600*24)
    time_jd[i]+=d_orb/(3600*24)
    mjd = Time(time[i],format='mjd', scale='tt')
    # direction to target
    dir_1957 = eph1957.pos(time[i])

    # Delay from and velocity of centre of earth to SSB (lt-s and v/c)
    posvel_earth = jpleph.compute('earth', time_jd[i])
    pos_earth = posvel_earth[:3]/c.to(u.km/u.s).value
    vel_earth = posvel_earth[3:]/c.to(u.km/u.day).value

    d_earth = np.sum(pos_earth*dir_1957, axis=0)
    v_earth = np.sum(vel_earth*dir_1957, axis=0)

    time[i]+=d_earth/(3600*24)
    time_jd[i]+=d_earth/(3600*24)
    mjd = Time(time[i],format='mjd', scale='tt')
    
    #Jodrell from tempo2-2013.3.1/T2runtime/observatory/observatories.dat
    xyz_jodrell = (3822626.04, -154105.65, 5086486.04)
    # Rough delay from observatory to center of earth
    # mean sidereal time (checked it is close to rf_ephem.utc_to_last)
    lmst = (observability.time2gmst(mjd)/24. + mjd.lon/360.)*2.*np.pi
    coslmst, sinlmst = np.cos(lmst), np.sin(lmst)
    # rotate observatory vector
    xy = np.sqrt(xyz_jodrell[0]**2+xyz_jodrell[1]**2)
    pos_jodrell = np.array([xy*coslmst, xy*sinlmst,
                         xyz_jodrell[2]*np.ones_like(lmst)])/c.si.value
    vel_jodrell = np.array([-xy*sinlmst, xy*coslmst,
                          np.zeros_like(lmst)]
                        )*2.*np.pi*366.25/365.25/c.to(u.m/u.day).value
    # take inner product with direction to pulsar
    d_topo = np.sum(pos_jodrell*dir_1957, axis=0)
    v_topo = np.sum(vel_jodrell*dir_1957, axis=0)

    time[i]+=d_topo/(3600*24)
    time_jd[i]+=d_topo/(3600*24)
    mjd = Time(time[i],format='mjd', scale='tt')

    #total relative velocity
    total_rv = - v_topo - v_earth + v_orb

    L=(1/(1+total_rv))#multiplying factor to find doppler frequency
    f_dp=f_p*L #doppler shifted frequency
    P_dp=1./f_dp #doppler shifted period
    d_doppler=P_dp-P_0 #delay due to doppler shift
    
    time[i]+=d_doppler/(3600*24)
    time_jd[i]+=d_doppler/(3600*24)
    mjd = Time(time[i],format='mjd', scale='tt').mjd
    arrival.append(time[i])

t = Time(arrival,format='mjd',scale='tt',precision=9)
r = Time(MJD_TDB, format='mjd',scale='tt',precision=9)
diff=[]
for i in range(len(arrival)):
    point=arrival[i]-MJD_TDB[i]
    diff.append(point)
time_arrival=t.iso
original_time=r.iso
with open("GenFromStapper.dat","w") as data:
    for i in range(len(time_arrival)):
        data.write("{0}\t{1}\t{2}\n".format(time_arrival[i],original_time[i],diff[i]))