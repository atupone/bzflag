import os
from bzfsItf import *

from CronJob import CronJob

BCVERSION = 'bzfscron 1.0.0'

class CronManager(bz_Plugin, bz_CustomSlashCommandHandler):
    jobs       = []
    lastTick   = 0.0
    lastMinute = -1
    crontab    = ''
    player     = None

    def Name(self):
        return 'BZFS Cron'

    def Init(self, commandLine):
        # should have a filename on the command line.  try to open it.
        if not commandLine:
            bz_debugMessage(1, 'bzfscron: no crontab specified')
            return

        self.crontab = commandLine

        if not self.reload():
            return

        # we have a granularity of 1 minute but we want to run things ASAP in that minute
        # since we rely on "real time" and not "relative time". n+<=5 sec should not hurt.
        bz_setMaxWaitTime(5.0)

        # register to receive ticks
        self.Register(bz_eTickEvent)

        # register /cron
        bz_registerCustomSlashCommand('cron', self)

        bz_debugMessage(4, BCVERSION + ': plugin loaded')

        if not self.connect():
            bz_debugMessage(1, BCVERSION + ': fake player could not connect!')
        else:
            bz_debugMessage(4, BCVERSION + ': fake player connected')

    def Cleanup(self):
        self.Flush()
        bz_removeCustomSlashCommand('cron')
        if self.player:
            bz_removeServerSidePlayer(self.player.getPlayerID())
            self.player = None

        bz_debugMessage(4, BCVERSION + ': plugin unloaded')

    def Event(self, event):
        if event.eventType != bz_eTickEvent:
            return event

        eventTime = bz_getCurrentTime()

        # ignore ticks that are less than 5 seconds apart
        if self.lastTick + 4.95 > eventTime:
            return
        self.lastTick = eventTime
        bz_debugMessage(4, 'bzfscron: tick!')

        # ensure that the minute has changed
        t = bz_getLocaltime()
        if t.minute == self.lastMinute:
            return
        self.lastMinute = t.minute
        bz_debugMessage(4, 'bzfscron: minute change')

        # make sure we have a valid player
        if not (self.player and self.player.valid()):
            return

        # iterate through all the jobs.  if they match the current minute, run them.
        for job in self.jobs:
            if job.matches(t.minute, t.hour, t.day, t.month, t.dow):
                bz_debugMessage(4, 'bzfscron: job matched at {}-{}-{} {}:{} - "{}"'.\
                        format(t.year, t.month, t.day, t.hour, t.minute,
                    job.getCommand()))
                self.player.sendCommand(job.getCommand())

    def SlashCommand(self, playerID, command, message, params):
        if not bz_hasPerm(playerID, 'BZFSCRON'):
            bz_sendTextMessage(playerID, \
                    'bzfscron: you do not have permission to run the /cron command.')
            return True
        if not len(params):
            bz_sendTextMessage(playerID, 'usage: /cron [list|reload]')
            return True
        param = params[0].lower()
        if param == 'reload':
            if self.reload():
                bz_sendTextMessage(playerID, 'bzfscron: reload succeeded.')
            else:
                bz_sendTextMessage(playerID, 'bzfscron: reload failed.')
        elif param == 'list':
            self.listCron(playerID);
        return True;

    def connect(self):
        # Create fake player
        self.player = CronPlayer()
        self.player.playerID = bz_addServerSidePlayer(self.player)
        return self.player.playerID >= 0

    def reload(self):
        # open the crontab
        if  not os.path.isfile(self.crontab) or os.path.getsize(self.crontab) <= 0:
            bz_debugMessage(1, 'bzfscron: crontab nonexistant or invalid')
            return False

        # clear current jobs
        self.jobs.clear()

        with open(self.crontab, 'r') as input:
            # read in the crontab
            for buffer in input:
                if buffer.startswith('#'):
                    continue
                newcron = CronJob(buffer)
                self.jobs.append(newcron)

        return True

    def listCron(self, playerID):
        for job in self.jobs:
            bz_sendTextMessage(playerID, job.displayJob().replace('\t', ' '))

class CronPlayer():
    playerID = -1

    def getPlayerID(self):
        return self.playerID

    def added(self, player):
        if player != self.playerID:
            return

        # oh look, it's ME!
        # set my information
        setPlayerData('bzfscron', '', BCVERSION, eObservers)
    
        # I think I'll make myself admin, so I can run all sorts of fun commands
        if not bz_setPlayerOperator(self.playerID):
            bz_debugMessage(1, 'bzfscron: unable to make myself an administrator')
        
        # But nobody needs to know I'm admin, 'cause I can't do anything unless crontab told me to
        bz_grantPerm(self.playerID, bz_perm_hideAdmin)

    def playerRejected(self, _, reason):
        temp = 'Player rejected (reason: {})'.format(reason)
        bz_debugMessage(1, temp)

    def sendCommand(self, message): # expose inherited protected member sendChatMessage
        temp = 'bzfscron: Executing "{}"'.format(message)
        bz_debugMessage(2, temp)
        sendServerCommand(message)

    def valid(self):
        return self.playerID >= 0;

plugin = CronManager()
