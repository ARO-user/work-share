import numpy as np
import astropy
from astropy.constants import c
from astropy import coordinates as coord
from astropy import units as u

locations=[[687.88 ,  -21.19], [326.43 ,  -42.67], [0 ,  0],[-372.72  , 141.29],
[-565.94  , 130.54], [67.82 , -260.80 ], [-31.44 , -233.29 ],[280.67 , -423.40 ], [41.92 , -160.39 ], [-164.88 , -621.34 ], [-603.28 , -340.09 ], [174.85 , -672.16 ], [-639.53 ,  -1182.34 ], [-473.71 ,  -664.85 ], [-1591.94 ,  625.39], [-3099.41 , 1501.18], [-5199.90 , 3066.16 ], [-7039.03 , 5359.52], [-8103.13 , 8267.57 ], [-11245.60 , 9430.02 ], [2814.55 , 1008.62 ], [4576.00 , 2043.81], [7780.69  ,  3070.58], [10200.00 , 3535.84 ], [12073.46 , 4804.91 ], [633.92 ,-2967.61 ], [-367.30 ,-4525.73 ], [333.03 , -6774.02 ], [947.68 , -9496.90 ], [-369.04 , -14153.59]]

distance=[]
for i in range(len(locations)):
    values=locations[i]
    dist=np.sqrt(((values[0])**2)+((values[1])**2))
    distance.append(dist)

delay=[]
for i in range(len(distance)):
    lt_s=distance[i]/c
    delay.append(lt_s)


#For more accurate results:
#input (x,y,z)

#sky=coord.ICRSCoordinates("19:59:36.7904934 20:48:14.90465", unit=(u.hour,u.degree))
#sky=str(sky)
#n=(sky.find('RA='))+3

deg_rad=np.pi/180

H_deg=299.90329
d_deg=20.80414
L_deg=19.0833
H=H_deg*deg_rad
d=d_deg*deg_rad
L=L_deg*deg_rad

locations=np.loadtxt('coords64_dec2.dat')
x=locations[:,0]
y=locations[:,1]
z=locations[:,2]

X=x
Y=y*np.cos(L)-z*np.sin(L)
Z=y*np.sin(L)+z*np.cos(L)

locations=[]
i=0
while i<63:
    m=X[i]*np.sin(H)+Y[i]*np.cos(H)
    n=-X[i]*np.sin(d)*np.cos(H)+Y[i]*np.sin(d)*np.sin(H)+Z[i]*np.cos(d)
    o=X[i]*np.cos(d)*np.cos(H)-Y[i]*np.cos(d)*np.sin(H)+Z[i]*np.sin(d)
    point=(m/c,n/c,o/c)
    if point not in locations:
        locations.append(point)
    else:
        pass
    i+=1
