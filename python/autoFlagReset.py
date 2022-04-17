from bzfsItf import *

class autoFlagReset(bz_Plugin):
    incremental = False
    freq        = 900
    nextRunTime = bz_getCurrentTime()
    nextFlag    = 0

    def Init (self, commandLine):
        if self.nextRunTime < 0.0:
            self.nextRunTime = 0.0

        if commandLine:
            if commandLine[-1] in 'iI':
                # Incremental mode.
                self.incremental = True
                commandLine = commandLine[:-1]

            newfreq = float(commandLine)
            if newfreq > 0.0:
               self.freq = newfreq * 60.0
        self.Register(bz_eTickEvent)

        bz_debugMessage(4, 'autoFlagReset plugin loaded')

    def Event(self, eventData):
        if eventData.eventType != bz_eTickEvent:
            return

        if bz_getCurrentTime() < self.nextRunTime:
            return

        nflags = bz_getNumFlags()
        if nflags == 0:
            return

        if self.incremental:
            # Reset one flag.
            # Limit iteration to one "cycle" of all flags.
            # Otherwise, this is an infinite loop if all flags are in use.
            for _ in range(nflags):
                worked = self.resetUnusedSuperflag(self.nextFlag)
                self.nextFlag += 1
                if self.nextFlag >= nflags:
                    self.nextFlag = 0
                if worked:
                    break
            self.nextRunTime += self.freq / float(nflags)
        else:
            # Reset all flags.
            for flagId in range(nflags):
                # Don't care whether it works or not.
                self.resetUnusedSuperflag(flagId)
            self.nextRunTime += self.freq

    def resetUnusedSuperflag(self, flagID):
        # Sanity check.
        if flagID >= bz_getNumFlags():
            return False

        # Make sure the flag isn't held.
        if bz_flagPlayer(flagID) != -1:
            return False

        # Make sure it's not a teamflag.
        flagType = bz_getFlagName(flagID)
        if flagType in ['R*', 'G*', 'B*', 'P*', '']:
            return False

        # Looks ok, reset it.
        bz_resetFlag(flagID)

        return True

plugin = autoFlagReset()
