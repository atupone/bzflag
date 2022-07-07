from enum import IntEnum
from bzBasic import *

bz_eNullEvent                   = 0
bz_eCaptureEvent                = 1
bz_ePlayerDieEvent              = 2
bz_ePlayerSpawnEvent            = 3
bz_eZoneEntryEvent              = 4
bz_eZoneExitEvent               = 5
bz_ePlayerJoinEvent             = 6
bz_ePlayerPartEvent             = 7
bz_eRawChatMessageEvent         = 8
bz_eFilteredChatMessageEvent    = 9
bz_eUnknownSlashCommand         = 10
bz_eGetPlayerSpawnPosEvent      = 11
bz_eGetAutoTeamEvent            = 12
bz_eAllowPlayer                 = 13
bz_eTickEvent                   = 14
bz_eGetWorldEvent               = 15
bz_eGetPlayerInfoEvent          = 16
bz_eAllowSpawn                  = 17
bz_eListServerUpdateEvent       = 18
bz_eBanEvent                    = 19
bz_eHostBanModifyEvent          = 20
bz_eKickEvent                   = 21
bz_eKillEvent                   = 22
bz_ePlayerPausedEvent           = 23
bz_eMessageFilteredEvent        = 24
bz_eGamePauseEvent              = 25
bz_eGameResumeEvent             = 26
bz_eGameStartEvent              = 27
bz_eGameEndEvent                = 28
bz_eSlashCommandEvent           = 29
bz_ePlayerAuthEvent             = 30
bz_eServerMsgEvent              = 31
bz_eShotFiredEvent              = 32
bz_ePlayerUpdateEvent           = 33
bz_eNetDataSendEvent            = 34
bz_eNetDataReceiveEvent         = 35
bz_eLoggingEvent                = 36
bz_eShotEndedEvent              = 37
bz_eFlagTransferredEvent        = 38
bz_eFlagGrabbedEvent            = 39
bz_eFlagDroppedEvent            = 40
bz_eAllowCTFCaptureEvent        = 41
bz_eMsgDebugEvent               = 42
bz_eNewNonPlayerConnection      = 43
bz_ePluginLoaded                = 44
bz_ePluginUnloaded              = 45
bz_ePlayerScoreChanged          = 46
bz_eTeamScoreChanged            = 47
bz_eWorldFinalized              = 48
bz_eReportFiledEvent            = 49
bz_eBZDBChange                  = 50
bz_eGetPlayerMotto              = 51
bz_eAllowConnection             = 52
bz_eAllowFlagGrab               = 53
bz_eAuthenticatonComplete       = 54
bz_eServerAddPlayer             = 55
bz_eAllowPollEvent              = 56
bz_ePollStartEvent              = 57
bz_ePollVoteEvent               = 58
bz_ePollVetoEvent               = 59
bz_ePollEndEvent                = 60
bz_eComputeHandicapEvent        = 61
bz_eBeginHandicapRefreshEvent   = 62
bz_eEndHandicapRefreshEvent     = 63
bz_eAutoPilotEvent              = 64
bz_eMuteEvent                   = 65
bz_eUnmuteEvent                 = 66
bz_eServerShotFiredEvent        = 67
bz_ePermissionModificationEvent = 68
bz_eAllowServerShotFiredEvent   = 69
bz_ePlayerDeathFinalizedEvent   = 70
bz_eLastEvent                   = 71

class bz_EventData():
    def __init__(self, eventType):
        self.eventType = eventType

class bz_TickEventData_V1 (bz_EventData):
    def __init__(self):
        super().__init__(bz_eTickEvent)

class bz_GetPlayerSpawnPosEventData_V1 (bz_EventData):
    def __init__(self, pos):
        super().__init__(bz_eGetPlayerSpawnPosEvent)
        self.pos = pos

class bz_ChatEventData_V1 (bz_EventData):
    def __init__(self, fromPlayer, message):
        super().__init__(bz_eRawChatMessageEvent)
        self.fromPlayer = fromPlayer
        self.message    = message

class bz_FlagTransferredEventData_V1 (bz_EventData):
    def __init__(self, flagType):
        super().__init__(bz_eFlagTransferredEvent)
        self.flagType = flagType

class bz_FlagGrabbedEventData_V1 (bz_EventData):
    def __init__(self, flagType, pos):
        super().__init__(bz_eFlagGrabbedEvent)
        self.flagType = flagType
        self.pos      = pos

class bz_FlagDroppedEventData_V1 (bz_EventData):
    def __init__(self, flagType):
        super().__init__(bz_eFlagDroppedEvent)
        self.flagType = flagType

class bz_ShotFiredEventData_V1 (bz_EventData):
    def __init__(self, playerID):
        super().__init__(bz_eShotFiredEvent)
        self.playerID = playerID

class bz_PlayerDieEventData_V2 (bz_EventData):
    def __init__(self, playerID, flagKilledWith):
        super().__init__(bz_ePlayerDieEvent)
        self.playerID       = playerID
        self.flagKilledWith = flagKilledWith

class bz_PlayerUpdateEventData_V1 (bz_EventData):
    def __init__(self, playerID, pos):
        super().__init__(bz_ePlayerUpdateEvent)
        self.playerID  = playerID
        self.state     = bz_PlayerUpdateState()
        self.state.pos = pos

class bz_AllowFlagGrabData_V1 (bz_EventData):
    allow = True

    def __init__(self, flagID, playerID):
        super().__init__(bz_eAllowFlagGrab)
        self.flagID   = flagID
        self.playerID = playerID

class bz_BasePlayerRecord():
    def __init__(self, team):
        self.team = team

class bz_PlayerJoinPartEventData_V1 (bz_EventData):
    def __init__(self, part, team):
        if part:
            super().__init__(bz_ePlayerPartEvent)
        else:
            super().__init__(bz_ePlayerJoinEvent)
        self.record = bz_BasePlayerRecord(bz_eTeamType(team))

# events handling

eventHandler = []
def RegisterEvent (eventType, plugin):
    eventHandler.append((eventType, plugin))

def RemoveEvent (eventType, plugin):
    eventHandler.remove((eventType, plugin))

def FlushEvents(plugin):
    for couple in eventHandler:
        if couple[1] == plugin:
            eventHandler.remove(couple)

def callEvents(event):
    for couple in eventHandler:
        if not isinstance(event, bz_EventData):
            break
        if couple[0] != event.eventType:
            continue
        event = couple[1].Event(event)
    return event
