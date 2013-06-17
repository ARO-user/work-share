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

data=LoadData('ForMartenCleaned')
del data[0]
del data[0]
MJD_TDB=IterativeFloatAppend(data,3)

eph1957 = ELL1Ephemeris('ForMarten.par')

MJD_TDB_NEW=eph1957.evaluate('F', MJD_TDB,t0par='PEPOCH',integrate=False)
