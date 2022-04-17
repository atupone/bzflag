from bzfsItf import *

chatHistories = {}

class LastChatCommand(bz_CustomSlashCommandHandler):

    def SlashCommand(plugin, playerID, command, message, params):
        # Only admins can run this command
        if not bz_getAdmin(playerID):
            bz_sendTextMessage(playerID, "You must be admin to use the ChatHistory plugin")
            return True

        global chatHistories
        # The 'last' command show the last X lines of text for a callsign
        if command == "last":
            # Must have two parameters
            if len(params) != 2:
                bz_sendTextMessage(playerID, "Usage: /last <NUMBER OF LINES> <CALLSIGN>")
                return True

            numLines, callSign = params

            numLines = int(numLines)
            # Parse the number of lines to return
            if numLines == 0:
                numLines = 5

            # Look up the player's chat history container
            # If the container doesn't exist or has a 0 size, bail out
            if callSign not in chatHistories:
                bz_sendTextMessage(playerID, "That player has no chat history.")
                return True

            # Store a reference to the chat history container
            history = chatHistories[callSign]

            # If the number of lines stored is less than the number requested, reduce the requested amount
            if len(history) < numLines:
                numLines = len(history)

            # Send the messages to the requestor
            bz_sendTextMessage(playerID, "Last {} message(s) for {}".format(numLines, callSign))
            for chatItem in reversed(history)[:numLines]:
                bz_sendTextMessage(playerID, "  <{}> {}".format(callSign, chatItem))

            return True

        # Clear all the chat histories
        if command == "flushchat":
            chatHistories = {}
            bz_sendTextMessage(playerID, "Chat History has been flushed")
            return True

        return False

lastChatCommand = LastChatCommand()

maxChatLines = 0

# event handler callback
class ChatEvents(bz_Plugin):
    def Name (self):
        return "Chat History"
    def Init (self, commandLine):

        global maxChatLines
        # Default to 50 lines per player
        maxChatLines = 50

        # Allow configuring how many lines to store
        if commandLine:
            maxChatLines = int(commandLine)

        # Register our custon slash commands
        bz_registerCustomSlashCommand('last',      lastChatCommand)
        bz_registerCustomSlashCommand('flushchat', lastChatCommand)

        # Register the raw chat event
        self.Register(bz_eRawChatMessageEvent)

    def Cleanup (self):
        # Remove our custom slash commands
        bz_removeCustomSlashCommand('last')
        bz_removeCustomSlashCommand('flushchat')

        # Remove our events
        self.Flush()
        pass

    def Event (self, chatEventData):
        # We only handle raw chat messages
        if chatEventData.eventType != bz_eRawChatMessageEvent:
            return

        global chatHistories

        # Retrieve the sender information
        fromPlayer = bz_getPlayerByIndex(chatEventData.fromPlayer)

        # Bail if we can't find the player
        if not fromPlayer:
            return event

        # Store the message
        message = chatEventData.message

        # Store the callsign
        callsign = fromPlayer.callsign.lower()

        # Get the old chat history for this callsign
        # or create a new chat history if necessary
        history = get(chatHistories[callsign], [])

        # Add the new message
        history.append(message)

        # Check if the number of chat history items exceeds the per-callsign limit.
        # If it does, remove the oldest entry
        if len(history) > maxChatLines:
            history.pop()

        # Store the chat history
        chatHistories[callsign] = history

plugin = ChatEvents()
