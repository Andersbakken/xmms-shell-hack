#include <stdio.h>
#include <xmmsctrl.h>
#include "config.h"
#include "command.h"

#define SECTION virtual const string get_section(void) const { return "Playback"; }

class PauseCommand : public Command
{
public:
	COM_STRUCT(PauseCommand, "pause")

	virtual void execute(CommandContext &cnx) const
	{
		int p;

		if(!xmms_remote_is_playing(cnx.session_id) && !xmms_remote_is_paused(cnx.session_id)) {
			printf("Pause command has no effect because no playback is currently in progress.\n");
			cnx.result_code = COMERR_NOEFFECT;
			return;
		}
		xmms_remote_pause(cnx.session_id);
		p = xmms_remote_is_paused(cnx.session_id);
		printf("Playback %spaused\n", p ? "un" : "");
		cnx.result_code = p;
	}

	COM_SYNOPSIS("pause or unpause playback")
	COM_SYNTAX("PAUSE")
	COM_DESCRIPTION(
		"The PAUSE command behaves as a toggle.  If XMMS is currently paused, the "
		"PAUSE command will have it resume playback.  If XMMS is currently playing, "
		"the PAUSE command will cause it to pause.  If no song is currently playing, "
		"the PAUSE command will have no effect.";
	)
	COM_RETURN(
		"If no playback is currently in progress, returns COMERR_NOEFFECT.  Otherwise "
		"returns 0 if playback is unpaused as a result of this command, and 1 otherwise.";
	)
	SECTION
};

class FakePauseCommand : public Command
{
public:
	COM_STRUCT(FakePauseCommand, "fakepause")

	virtual void execute(CommandContext &cnx) const
	{
		int pos, offset, ch;

		if(!xmms_remote_is_playing(cnx.session_id)) {
			printf("Playback is not currently in progress.\n");
			cnx.result_code = COMERR_NOEFFECT;
			return;
		}
		pos = xmms_remote_get_playlist_pos(cnx.session_id);
		offset = xmms_remote_get_output_time(cnx.session_id);
		xmms_remote_stop(cnx.session_id);
		printf("Press [ENTER] to resume: ");
		fflush(stdout);
		while((ch = getc(stdin)) != EOF && ch != '\n');
		xmms_remote_set_playlist_pos(cnx.session_id, pos);
		xmms_remote_play(cnx.session_id);
		xmms_remote_jump_to_time(cnx.session_id, offset);
		printf("Playback resumed.\n");
		cnx.result_code = 0;
	}

	virtual int get_flags(void) const { return COMFLAG_INTERACTIVE; }

	COM_SYNOPSIS("pause XMMS and release the output device")
	COM_SYNTAX("FAKEPAUSE")
	COM_DESCRIPTION(
		"The FAKEPAUSE is similar to the PAUSE command in that it pauses playback.  "
		"However, FAKEPAUSE is an interactive command.  When you press [ENTER] at "
		"the prompt FAKEPAUSE provides, playback will resume where it left off.  "
		"The usefulness of this command is that it forces XMMS to release the "
		"output device until you unpause it.  Technically it does this by saving "
		"the current song and the position in it before executing the STOP command.  "
		"Then, when it unpauses, it jumps back to the saved position in the song "
		"and resumes playback."
	)
	COM_RETURN(
		"If no playback is currently in progress, returns COMERR_NOEFFECT.  Otherwise "
		"returns 0."
	)
	SECTION
};

class PlayCommand : public Command
{
public:
	COM_STRUCT(PlayCommand, "play")

	virtual void execute(CommandContext &cnx) const
	{
		int p, q;

		p = xmms_remote_is_playing(cnx.session_id);
		q = xmms_remote_is_paused(cnx.session_id);
		xmms_remote_play(cnx.session_id);
		if(p) {
			if(q) {
				printf("Playback unpaused\n");
				cnx.result_code = 2;
			} else {
				printf("Playback started\n");
				cnx.result_code = 1;
			}
		} else {
			printf("Playback restarted\n");
			cnx.result_code = 0;
		}
	}

	COM_SYNOPSIS("start or resume playback")
	COM_SYNTAX("PLAY")
	COM_DESCRIPTION(
		"The PLAY command forces playback to begin or resume.  If a track is currently "
		"playing, this command has the effect of causing playback of the track to start "
		"over.  If playback is paused, PLAY resumes playback where it left off.  Otherwise "
		"playback begins at the beginning of the current track."
	)
	COM_RETURN(
		"If playback was previously paused, 2 is returned.  If a track was already being "
		"played when this command was issued, 1 is returned.  Otherwise 0 is returned."
	)
	SECTION
};

class StopCommand : public Command
{
public:
	COM_STRUCT(StopCommand, "stop")

	virtual void execute(CommandContext &cnx) const
	{
		int p, q;

		p = xmms_remote_is_playing(cnx.session_id);
		q = xmms_remote_is_paused(cnx.session_id);
		xmms_remote_stop(cnx.session_id);
		if(p) {
			if(q) {
				printf("Playback unpaused and stopped\n");
				cnx.result_code = 2;
			} else {
				printf("Playback stopped\n");
				cnx.result_code = 1;
			}
		} else {
			printf("Playback was already stopped\n");
			cnx.result_code = 0;
		}
	}

	COM_SYNOPSIS("stop playback")
	COM_SYNTAX("STOP")
	COM_DESCRIPTION(
		"The STOP command forces playback to stop.  If playback is subsequently resumed, "
		"it begins at the beginning of the current track."
	)
	COM_RETURN(
		"If playback was previously paused, 2 is returned.  If a track was being "
		"played when this command was issued, 1 is returned.  Otherwise 0 is returned."
	)
	SECTION
};

class RepeatCommand : public Command
{
public:
	COM_STRUCT(RepeatCommand, "repeat")
	
	virtual void execute(CommandContext &cnx) const
	{
#if HAVE_XMMS_REMOTE_IS_REPEAT
		gboolean status, newstatus;

		status = xmms_remote_is_repeat(cnx.session_id);
		if(cnx.args.size() < 2)
			newstatus = !status;
		else if(!strncasecmp("off", cnx.args[1].c_str(), cnx.args[1].size()))
			newstatus = FALSE;
		else if(!strncasecmp("on", cnx.args[1].c_str(), cnx.args[1].size()))
			newstatus = TRUE;
		else if(!strncasecmp("toggle", cnx.args[1].c_str(), cnx.args[1].size()))
			newstatus = !status;
		else {
			cnx.result_code = COMERR_SYNTAX;
			return;
		}
		cnx.result_code = newstatus;
		printf("Repeat mode is now: %s\n", cnx.result_code ? "on" : "off");
		if(status != newstatus)
#else
		cnx.result_code = 0;
#endif
		xmms_remote_toggle_repeat(cnx.session_id);
	}

#if HAVE_XMMS_REMOTE_IS_REPEAT
	COM_SYNOPSIS("set repeat mode")
	COM_SYNTAX("REPEAT [OFF|ON|TOGGLE]")
	COM_DESCRIPTION(
		"Clears, sets, or toggles repeat mode.  "
		"If neither OFF nor ON is given, TOGGLE is assumed to be the default."
	)
	COM_RETURN("Returns 1 if repeat mode is turned on, 0 otherwise")
#else
	COM_SYNOPSIS("toggle repeat mode")
	COM_SYNTAX("REPEAT")
	COM_DESCRIPTION(
		"Toggles repeat mode.  The version of XMMS that XMMS-Shell was compiled with "
		"does not support querying of the current repeat mode, so it is impossible to "
		"simply set or clear repeat mode."
	)
	COM_RETURN("Always 0")
#endif
	SECTION
};

class ShuffleCommand : public Command
{
public:
	COM_STRUCT(ShuffleCommand, "shuffle")
	
	virtual void execute(CommandContext &cnx) const
	{
#if HAVE_XMMS_REMOTE_IS_SHUFFLE
		gboolean status, newstatus;

		status = xmms_remote_is_shuffle(cnx.session_id);
		if(cnx.args.size() < 2)
			newstatus = !status;
		else if(!strncasecmp("off", cnx.args[1].c_str(), cnx.args[1].size()))
			newstatus = FALSE;
		else if(!strncasecmp("on", cnx.args[1].c_str(), cnx.args[1].size()))
			newstatus = TRUE;
		else if(!strncasecmp("toggle", cnx.args[1].c_str(), cnx.args[1].size()))
			newstatus = !status;
		else {
			cnx.result_code = COMERR_SYNTAX;
			return;
		}
		cnx.result_code = newstatus;
		printf("Shuffle mode is now: %s\n", cnx.result_code ? "on" : "off");
		if(status != newstatus)
#else
		cnx.result_code = 0;
#endif
		xmms_remote_toggle_shuffle(cnx.session_id);
	}

#if HAVE_XMMS_REMOTE_IS_SHUFFLE
	COM_SYNOPSIS("set shuffle mode")
	COM_SYNTAX("REPEAT [OFF|ON|TOGGLE]")
	COM_DESCRIPTION(
		"Clears, sets, or toggles shuffle mode.  "
		"If neither OFF nor ON is given, TOGGLE is assumed to be the default."
	)
	COM_RETURN("Returns 1 if shuffle mode is turned on, 0 otherwise")
#else
	COM_SYNOPSIS("toggle shuffle mode")
	COM_SYNTAX("REPEAT")
	COM_DESCRIPTION(
		"Toggles shuffle mode.  The version of XMMS that XMMS-Shell was compiled with "
		"does not support querying of the current shuffle mode, so it is impossible to "
		"simply set or clear shuffle mode."
	)
	COM_RETURN("Always 0")
#endif
	SECTION
};

static Command *commands[] = {
	new PauseCommand(),
	new FakePauseCommand(),
	new PlayCommand(),
	new StopCommand(),
	new RepeatCommand(),
	new ShuffleCommand(),
};

void playback_init(void)
{
	for(unsigned i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
		command_add(commands[i]);
}

