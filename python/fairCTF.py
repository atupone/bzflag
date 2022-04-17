from bzfsItf import *

class fairCTF (bz_Plugin, bz_CustomSlashCommandHandler):

    # Initialize defaults
    allowCTF     = True
    autoMode     = True
    max_ratio    = 0.25
    max_gap_by_1 = 2
    max_gap      = 3
    drop_delay   = 5
    droptime     = 0.0

    def Name (self):
        return 'Fair CTF'

    def Init (self, rawparams):
        # Parse out args
        params = []
        for _ in range(4):
            params.append('')

        n = 0

        for ch in rawparams:
            if ch == ':':
                n += 1
                if n > 3:
                    break
            else:
                params[n].append(ch)

        if params[0]:
            tempratio = float(params[0])
            if tempratio > 0.0:
                self.max_ratio = tempratio

        if params[1]:
            tempmax1gap = int(params[1])
            if tempmax1gap > 0:
                self.max_gap_by_1 = tempmax1gap

        if params[2]:
            tempmaxgap = int(params[2])
            if tempmaxgap > 0:
                self.max_gap = tempmaxgap

        if params[3]:
            tempdelay = int(params[3])
            if tempdelay > 0:
                self.drop_delay = tempdelay

        self.Register(bz_eAllowFlagGrab)
        self.Register(bz_ePlayerJoinEvent)
        self.Register(bz_ePlayerPartEvent)
        self.Register(bz_eTickEvent)

        bz_registerCustomSlashCommand ('ctf', self)

        bz_debugMessage(4, 'fairCTF plugin loaded')

        self.UpdateState(bz_eTeamType.eNoTeam)

    def Cleanup(self):
        self.Flush()
        bz_removeCustomSlashCommand ('ctf')

        bz_debugMessage(4, 'fairCTF plugin unloaded')

    def Event(self, eventData):
        if eventData.eventType == bz_eAllowFlagGrab:
            if self.allowCTF:
                return eventData

            # Don't allow a team flag grab
            flagtype = bz_getFlagName(eventData.flagID)
            if flagtype == "R*" or flagtype == "G*" or flagtype == "B*" or \
                    flagtype == "P*":
                eventData.allow = False
                bz_sendTextMessage (eventData.playerID, \
                        "CTF play is currently disabled.")
        elif eventData.eventType == bz_ePlayerJoinEvent:
            self.UpdateState(bz_eTeamType.eNoTeam)
        elif eventData.eventType == bz_ePlayerPartEvent:
            # Need to compensate for that leaving player.
            self.UpdateState(eventData.record.team)
        elif eventData.eventType == bz_eTickEvent:
            if self.droptime == 0.0:
                return eventData
            if bz_getCurrentTime() < self.droptime:
                return eventData

            # Time to drop any team flags.
            for playerID in bz_getPlayerIndexList():
                self.DropTeamFlag(playerID)

            self.droptime = 0.0
        return eventData

    def SlashCommand (self, playerID, _, message, __):
        cs = 'UNKNOWN'
        pr = bz_getPlayerByIndex(playerID)
        if pr:
            cs = pr.callsign

        if not bz_hasPerm(playerID, 'FAIRCTF'):
            bz_sendTextMessage(playerID, cs + ', you do not have permission to use the /ctf command.')
            return True

        if message == 'on':
            if not self.autoMode and self.allowCTF:
                bz_sendTextMessage(playerID, 'CTF is already set to "on".')
                return True
        elif message == 'off':
            if not self.autoMode and not self.allowCTF:
                bz_sendTextMessage(playerID, 'CTF is already set to "off".')
                return True
        elif message == 'auto':
            if self.autoMode:
                bz_sendTextMessage(playerID, 'CTF is already set to "auto".');
                return True

        if message == 'on':
            self.autoMode = False
            bz_sendTextMessage (bz_eTeamType.eAdministrators, 'CTF setting has been changed to "on" by ' + cs + '.')
            if not self.allowCTF:
                bz_sendTextMessage (BZ_ALLUSERS, 'CTF has been enabled by ' + cs + '.')
                self.allowCTF = True
                self.droptime = 0.0
        elif message == 'off':
            self.autoMode = False
            bz_sendTextMessage (bz_eTeamType.eAdministrators, 'CTF setting has been changed to "off" by ' + cs + '.')
            if self.allowCTF:
                bz_sendTextMessage (BZ_ALLUSERS, 'CTF has been disabled by ' + cs + '.')
                self.allowCTF = False
                self.SetDropTime()
        elif message == 'auto':
            self.autoMode = True
            bz_sendTextMessage (bz_eTeamType.eAdministrators, 'CTF setting has been changed to "auto" by ' + cs + '.')
            self.UpdateState(bz_eTeamType.eNoTeam)
        else:
            bz_sendTextMessage (playerID, 'Usage: /ctf on|off|auto');
        return True

    def DropTeamFlag(self, playerID):
        droppr = bz_getPlayerByIndex (playerID)

        if not droppr:
            return

        # Are they carrying a team flag?
        if (droppr.currentFlag != "Red team flag") and\
                (droppr.currentFlag != "Green team flag") and\
                (droppr.currentFlag != "Blue team flag")\
                and (droppr.currentFlag != "Purple team flag"):
                    return

        bz_removePlayerFlag(playerID)
        bz_sendTextMessage (playerID, "CTF play is currently disabled.")

    def SetDropTime(self):
        # is any tank carrying a team flag?
        TeamFlagIsCarried = False
        for player in bz_getPlayerIndexList():
            FlagHeld = bz_getPlayerFlag(player)
            if FlagHeld in ('R*', 'G*', 'B*', 'P*'):
                TeamFlagIsCarried = True
                break

        # announce drop delay only if some tank is carrying a team flag
        if not TeamFlagIsCarried:
            return

        if self.drop_delay < 0:
            bz_sendTextMessage(BZ_ALLUSERS,
                    'Currently-held team flags will not be dropped.')
            return

        self.droptime = bz_getCurrentTime() + self.drop_delay
        if self.drop_delay > 1:
            bz_sendTextMessage(BZ_ALLUSERS,\
                    'Currently-held team flags will be dropped in {} seconds.'\
                    .format(self.drop_delay))
        else:
            bz_sendTextMessage(BZ_ALLUSERS,\
                    'Currently-held team flags will be dropped in 1 second.')

    def UpdateState(self, teamLeaving):
        if not self.autoMode:
            return
        fair = self.isEven(teamLeaving)

        if fair and not self.allowCTF:
            self.allowCTF = True
            self.droptime = 0.0
            bz_sendTextMessage (\
                    BZ_ALLUSERS,\
                    'Team sizes are sufficiently even. ' + \
                    'CTF play is now enabled.')
        elif not fair and self.allowCTF:
            self.allowCTF = False
            bz_sendTextMessage(\
                    BZ_ALLUSERS,\
                    'Team sizes are uneven. CTF play is now disabled.')

            SetDropTime()

    def isEven(self, teamLeaving):
        teamsizes = []

        teamsizes.append(bz_getTeamCount (bz_eTeamType.eRedTeam))
        teamsizes.append(bz_getTeamCount (bz_eTeamType.eGreenTeam))
        teamsizes.append(bz_getTeamCount (bz_eTeamType.eBlueTeam))
        teamsizes.append(bz_getTeamCount (bz_eTeamType.ePurpleTeam))

        leavingTeamIndex = int(teamLeaving)
        if leavingTeamIndex >= 1 and leavingTeamIndex <= 4:
            # Decrement the team count for the player that's leaving the game.
            teamsizes[leavingTeamIndex - 1] -= 1

        #check fairness

        smallestTeam = 10000 #impossibly high
        largestTeam  = 0

        for teamSize in teamsizes:
            if teamSize > largestTeam:
                largestTeam = teamSize
            if teamSize != 0 and teamSize < smallestTeam:
                smallestTeam = teamSize

        #check differences and ratios

        if smallestTeam == 10000 or largestTeam == smallestTeam:
            #equal, or server has no team tanks
            return True
        if smallestTeam <= self.max_gap_by_1:
            # user-defined cap on a difference of 1
            return False
        if largestTeam - smallestTeam == 1: #after UD limit
            return True
        if (largestTeam - smallestTeam) / smallestTeam > self.max_ratio:
            #greater than specified gap
            return False
        if largestTeam - smallestTeam >= self.max_gap:
            return False

        return True

plugin = fairCTF()
