from bzfsItf import *

# The custom zone we'll be making for this plug-in and we'll be extending a
# class provided by the API.
# The class provided in the API handles all of the logic handling rectangular
# and circular zones.
class MsgZone (bz_CustomZoneObject):
    # Our custom constructor will call the parent constructor so we can setup
    # default positions for the zone
    flag    = 'US'

    # Custom fields that are unique to our zone so we can build on top of the
    # class we're extending
    message = ''

class CustomZoneSample (bz_Plugin, bz_CustomMapObjectHandler):
    msgZones = []

    def Name (self):
        return 'Custom Zone Sample'

    def Init (self, _):
        self.Register(bz_ePlayerUpdateEvent)

        # Whenever a player enters a zone and is carrying a specified flag,
        # they will receive the specified message
        bz_registerCustomMapObject('msgzone', self)


    def Event (self, updateData):
        if updateData.eventType != bz_ePlayerUpdateEvent:
            return

        # This event is called each time a player sends an update to the server
        # Loop through all of our custom zones
        for msgZone in self.msgZones:
            # Use the pointInZone(float pos[3]) function provided by the
            # bz_CustomZoneObject to check if the position
            # of the player is inside of the zone. This function will
            # automatically handle the logic if the zone is a
            # rectangle (even if it's rotated) or a circle
            if msgZone.pointInZone(updateData.state.pos) and \
                    bz_getPlayerFlagID(updateData.playerID) >= 0:
                # If the player has the flag specified in the zone, send them
                # a message and remove their flag
                if bz_getPlayerFlag(updateData.playerID) == msgZone.flag:
                    bz_sendTextMessage(updateData.playerID, msgZone.message)
                    bz_removePlayerFlag(updateData.playerID);

    def Cleanup (self):
        self.Flush();

        bz_removeCustomMapObject('msgzone');

    def MapObject (self, object, data):
        if object != 'MSGZONE' or not data:
            return False

        # The new zone we just found and we'll be storing in our vector of
        # zones
        newZone = MsgZone()

        # This function will parse the attributes that are handled by
        # bz_CustomZoneObject which
        # handles rectangular and circular zones
        #
        # For rectangular zones:
        #   - position
        #   - size
        #   - rotation
        #
        # For circular zones:
        #   - position
        #   - height
        #   - radius
        #
        # This also handles BBOX and CYLINDER fields but they have been
        # deprecated and will be removed in the future
        newZone.handleDefaultOptions(data)

        # Loop through the object data
        for line in data:
            nubs = bz_tokenize(line)

            if len(nubs) <= 1:
                continue

            if len(nubs[1]) >=2 and nubs[1][0] == '"':
                nubs[1] = nubs[1][1:-1]

            key = nubs[0].upper()

            # These are our custom fields in the MsgZone class
            if key == 'MESSAGE':
                newZone.message = nubs[1]
            elif key == 'FLAG':
                newZone.flag = nubs[1]

        self.msgZones.append(newZone)

        return True

plugin = CustomZoneSample()
