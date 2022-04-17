from bzfsItf import *

class CustomFlagSample (bz_Plugin):
    def Name (self):
        return 'Custom Flag Sample'

    def Init (self, commandLine):
        bz_debugMessage(4, 'customflagsample plugin loaded')

        # register our special custom flag
        bz_RegisterCustomFlag('CF', 'Custom Flag',
                'A simple sample custom flag from the customflagsample plugin', 0, eGoodFlag)

        # register events for pick up, drop, transfer, and fire
        self.Register(bz_eFlagTransferredEvent)
        self.Register(bz_eFlagGrabbedEvent)
        self.Register(bz_eFlagDroppedEvent)
        self.Register(bz_eShotFiredEvent)
        self.Register(bz_ePlayerDieEvent)

    def Cleanup (self):
        # unregister our events
        self.Flush()

        bz_debugMessage(4, 'customflagsample plugin unloaded')

    def Event (self, eventData):
        if eventData.eventType == bz_eFlagTransferredEvent:
            if eventData.flagType == 'CF':
                bz_sendTextMessage(BZ_ALLUSERS, 'Custom Flag transferred!')
        elif eventData.eventType == bz_eFlagGrabbedEvent:
            if eventData.flagType == 'CF':
                bz_sendTextMessage(BZ_ALLUSERS, 'Custom Flag grabbed!')
        elif eventData.eventType == bz_eFlagDroppedEvent:
            if eventData.flagType == 'CF':
                bz_sendTextMessage(BZ_ALLUSERS, 'Custom Flag dropped!')
        elif eventData.eventType == bz_eShotFiredEvent:
            p = eventData.playerID
            playerRecord = bz_getPlayerByIndex(p)
            if playerRecord:
                if playerRecord.currentFlag == 'Custom Flag (+CF)':
                    bz_sendTextMessage(BZ_ALLUSERS, 'Shot fired by {} with Custom Flag!'.format(playerRecord.callsign))
                    # this user must be cool, add 10 to their score
                    bz_incrementPlayerWins(p, 10)
        elif eventData.eventType == bz_ePlayerDieEvent:
            flag = eventData.flagKilledWith
            p    = eventData.playerID
            playerRecord = bz_getPlayerByIndex(p)
            if flag == 'CF':
                bz_sendTextMessage(BZ_ALLUSERS, 'Player {} killed by a player with Custom Flag!'.format(playerRecord.callsign))
        else:
            # no, sir, we didn't ask for THIS!!
            bz_debugMessage(1, 'customflagsample: received event with unrequested eventType!')
        return eventData

plugin = CustomFlagSample()
