import numpy as np
from astropy.time import Time

def LoadData(fname):
    f=open(fname,'r')
    data=[]
    for line in f.readlines():
        data.append(line.replace('\n','').split(' '))
    f.close()
    return data

def IterativeFloatAppend(values,index):
    new_list=[]
    for i in range((len(values))-1):
        point=float(values[i][index])
        new_list.append(point)
    return new_list

def Differences(values):
    new_list=[]
    for i in range((len(values))-1):
        point=values[i+1]-values[i]
        new_list.append(point)
    return new_list

def MjdToIso(values):
    ist=[]
    for i in range(len(values)):
        ist_fix=values[i]+(5.5/24)
        ist.append(ist_fix)
    T=Time(ist,format='mjd',scale='utc',precision=9)
    new_list=T.iso
    return new_list

def RemoveRepeats(values):
    new_list=[]
    for i in range(len(values)):
        if values[i] not in new_list:
            new_list.append(values[i])
    return new_list

def WriteFile(values,fname):
    with open(fname,"w") as data:
        for i in range(len(values)):
            data.write("%s\n"%values[i])

#EXAMPLE - ACCESSING JUNE 2 DATA

#Data=LoadData('/cita/h/home-2/njones/work-share/Summer2013/Ephemeris/ForMartenCleaned')


#start=2
#stop=106
#June2012=Data[start:stop]
#Dates=IterativeFloatAppend(June2012,3)
#Times=MjdToIso(Dates)
#WriteFile(Times,"02Jun2012real.dat")
