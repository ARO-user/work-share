import numpy as np
import manage as man
import matplotlib.pyplot as plt

data=man.LoadData('pycoords60_2013.dat')
x=man.IterativeFloatAppend(data,0)
y=man.IterativeFloatAppend(data,1)
z=man.IterativeFloatAppend(data,2)
#plt.plot(x,z,'.',color='blue')
test=man.LoadData('../4minMay16first1919UVW.dat')
u=man.IterativeFloatAppend(test,4)
v=man.IterativeFloatAppend(test,6)
w=man.IterativeFloatAppend(test,8)
#plt.plot(u,w,'.',color='red')
test=man.LoadData('../4minMay16second1919UVW.dat')
u=man.IterativeFloatAppend(test,4)
v=man.IterativeFloatAppend(test,6)
w=man.IterativeFloatAppend(test,8)
#plt.plot(u,w,'.',color='green')
test=man.LoadData('../4minMay16_1957UVW.dat')
u=man.IterativeFloatAppend(test,4)
v=man.IterativeFloatAppend(test,6)
w=man.IterativeFloatAppend(test,8)
#plt.plot(u,w,'.',color='purple')
test=man.LoadData('../June29_0329UVWfixedtimes.dat')
u=man.IterativeFloatAppend(test,4)
v=man.IterativeFloatAppend(test,6)
plt.plot(u,v,'.',color='blue')
plt.show()
