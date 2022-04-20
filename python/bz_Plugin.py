from bzEvents import *

class bz_Plugin():
    MaxWaitTime = -1
    Unloadable  = True

    def Cleanup(self):
        pass

    def Event(self, eventData):
        return eventData

    # used for inter plugin communication
    def GeneralCallback(self, name, data):
        return 0

    def Register (self, eventType):
        RegisterEvent(eventType, self)

    def Remove (eventType):
        RemoveEvent(eventType, self)

    def Flush (self):
        FlushEvents(self)
