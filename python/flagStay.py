from bzfsItf import *

class FlagStayZone(bz_CustomZoneObject):
    message  = ''
    flagList = []

    def checkFlag (self, flag):
        return flag in self.flagList

zoneList = []

class FlagStay (bz_Plugin, bz_CustomMapObjectHandler):

    def Name (self):
        return 'Flag Stay Zones'
    def __init__ (self, _):
        self.playerIDToZoneMap = {}

        bz_registerCustomMapObject('FLAGSTAYZONE', self)

        self.Register(bz_eFlagGrabbedEvent)
        self.Register(bz_ePlayerUpdateEvent)

    def Cleanup (self):
        self.Flush()

        bz_removeCustomMapObject('FLAGSTAYZONE')

    def Event (self, eventData):
        if eventData.eventType == bz_eFlagGrabbedEvent:
            flagGrabData = eventData

            for zone in zoneList:
                if not zone.pointInZone(flagGrabData.pos):
                    continue
                if not zone.checkFlag(flagGrabData.flagType):
                    continue
                self.playerIDToZoneMap[flagGrabData.playerID] = zone
                break
        elif eventData.eventType == bz_ePlayerUpdateEvent:
            updateData = eventData
            playerID   = updateData.playerID

            if playerID not in self.playerIDToZoneMap:
                return

            pos = updateData.state.pos

            flagAbrev = bz_getPlayerFlag(playerID)

            if not flagAbrev:
                del self.playerIDToZoneMap[playerID]
            else:
                zone = self.playerIDToZoneMap[playerID]

                if not zone.pointInZone(pos):
                    bz_removePlayerFlag(playerID)
                    del self.playerIDToZoneMap[playerID]

                    if zone.message.size():
                        bz_sendTextMessage(playerID, zone.message)

    def MapObject (self, obj, data):
        if obj != 'FLAGSTAYZONE':
            return False
        if not data:
            return False

        newZone = FlagStayZone()
        newZone.handleDefaultOptions(data)

        # parse all the chunks
        for line in data:
            nubs = bz_tokenize(line)

            if len(nubs) <= 1:
                continue

            key = nubs[0].upper()

            if key == 'FLAG':
                flag = nubs[1].upper()
                newZone.flagList.append(flag)
            elif key == 'MESSAGE' or key == 'MSG':
                newZone.message = nubs[1]

        zoneList.append(newZone)

        return True

plugin = FlagStay('')
