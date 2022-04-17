from bzfsItf import *

class customPollTypeSample (bz_Plugin, bz_CustomPollTypeHandler):
    def Name (self):
        return 'Custom Poll Type'

    def Init (self, _):
        # This will introduce a '/poll mute callsign' option
        bz_registerCustomPollType('mute', 'callsign', self)

    def Cleanup (self):
        self.Flush();

        # Remove the poll option when this plugin is loaded or else what other
        # plugin would handle it?
        bz_removeCustomPollType("mute");

    def PollOpen (self, player, action, _):
        # This function is called before a `/poll mute <callsign>` poll is
        # started. If this function returns false, then the poll will not
        # start. This is useful for checking permissions or other conditions.
        playerID = player.playerID;
        _action  = action;

        # If a player doesn't have the 'poll' permission, they will not be able to start a poll. Be sure to send the playerID a message
        # or else it'll appear as if the /poll command did not work.
        if not bz_hasPerm(playerID, 'pollMute'):
            bz_sendTextMessage(playerID, "You can't start a poll!")
            return False

        # The 'action' variable will be set whichever poll option is being called
        if _action == 'mute':
            # Return true in order to let BZFS start the poll
            return True

        # This should never be reached but it'll take care of compiler warnings
        return False

    def PollClose (self, action, parameters, success):
        _action     = action
        _parameters = parameters

        if _action == 'mute' and success:
            pr = bz_getPlayerBySlotOrCallsign(_parameters)

            if not pr:
                bz_sendTextMessage(BZ_ALLUSERS, 'player {} not found'.format(_parameters))
                return

            # Poll succeeded, so mute the player
            bz_revokePerm(pr.playerID, "talk");

plugin = customPollTypeSample()
