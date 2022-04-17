# airSpawn

from random import random

from bzfsItf import *

class airspawn(bz_Plugin):
    spawnRange = 0.0

    def Init (self, commandLine):
        if commandLine:
            self.spawnRange = float(commandLine)
        if self.spawnRange < 0.001:
            self.spawnRange = 10.0
        self.Register(bz_eGetPlayerSpawnPosEvent)

    def Event(self, eventData):
        if eventData.eventType != bz_eGetPlayerSpawnPosEvent:
            return
        eventData.pos.z += random() * self.spawnRange
        return eventData

plugin = airspawn()
