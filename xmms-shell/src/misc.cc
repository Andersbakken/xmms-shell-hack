#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <xmmsctrl.h>
#include "command.h"

#define SECTION virtual const string get_section(void) const { return "Playlist"; }

class ResetDeviceCommand : public Command
{
public:
	COM_STRUCT(ResetDeviceCommand, "resetdevice")
	
	virtual void execute(CommandContext &cnx) const
	{
		gint pos, offset;
		gboolean paused;

		cnx.result_code = 0;
		if(!xmms_remote_is_playing(cnx.session_id))
			return;

		paused = xmms_remote_is_paused(cnx.session_id);
		pos = xmms_remote_get_playlist_pos(cnx.session_id);
		offset = xmms_remote_get_output_time(cnx.session_id);
		xmms_remote_stop(cnx.session_id);
		xmms_remote_set_playlist_pos(cnx.session_id, pos);
		xmms_remote_play(cnx.session_id);
		xmms_remote_jump_to_time(cnx.session_id, offset);
		if(paused)
			xmms_remote_pause(cnx.session_id);
	}

	COM_SYNOPSIS("force XMMS to reset the output device")
	COM_SYNTAX("RESETDEVICE")
	COM_DESCRIPTION(
		"The RESETDEVICE command forces XMMS to reset the output device.  This is "
		"useful if you wish to apply new output device settings immediately without "
		"interrupting playback.  This is technically implemented in the same manner "
		"as the FAKEPAUSE command."
	)
	COM_RETURN("Always 0")
	SECTION
};

class FadeCommand : public Command
{
public:
	COM_STRUCT(FadeCommand, "fade")

	virtual void execute(CommandContext &cnx) const
	{
		gint target = 0, step = 1, delay = 100000, channel = 2, pos;
		gint lv, rv, olv, orv, v;
		int x = 1;
	
		if(!xmms_remote_is_playing(cnx.session_id)) {
			fprintf(stderr, "No song is playing, ignoring fade request\n");
			cnx.result_code = 0;
			return;
		}
		if(xmms_remote_is_paused(cnx.session_id)) {
			fprintf(stderr, "Song is paused, ignoring fade request\n");
			cnx.result_code = 0;
			return;
		}
		cnx.result_code = 1;
	
		if(cnx.args.size() > 1) {
			if(!strncasecmp("left", cnx.args[1].c_str(), cnx.args[1].length())) {
				channel = 0;
				x++;
			} else if(!strncasecmp("right", cnx.args[1].c_str(), cnx.args[1].length())) {
				channel = 1;
				x++;
			}
		}
	
		pos = xmms_remote_get_playlist_pos(cnx.session_id);
		xmms_remote_get_volume(cnx.session_id, &lv, &rv);
	
		if(lv > rv)
			v = lv;
		else
			v = rv;
	
		olv = lv;
		orv = rv;
		
		if(cnx.args.size() - x > 0) {
			if(!isdigit(cnx.args[x][0])) {
				cnx.result_code = COMERR_SYNTAX;
				return;
			}
			target = atoi(cnx.args[x].c_str());
			if(cnx.args.size() - x > 1) {
				if(!isdigit(cnx.args[x + 1][0])) {
					cnx.result_code = COMERR_SYNTAX;
					return;
				}
				step = atoi(cnx.args[x + 1].c_str());
				if(!step) {
					fprintf(stderr, "The stepsize must be greater than zero.\n");
					cnx.result_code = COMERR_SYNTAX;
					return;
				}
				if(cnx.args.size() - x > 2) {
					if(!isdigit(cnx.args[x + 2][0])) {
						cnx.result_code = COMERR_SYNTAX;
						return;
					}
					delay = atoi(cnx.args[x + 2].c_str());
				}
			}
		}
		if(v > target)
			while(v > target && xmms_remote_get_playlist_pos(cnx.session_id) == pos) {
				v -= step;
				if(channel != 1)
					lv -= step;
				if(channel != 0)
					rv -= step;
				xmms_remote_set_volume(cnx.session_id, lv, rv);
				usleep(delay);
			}
		else
			while(v < target && xmms_remote_get_playlist_pos(cnx.session_id) == pos) {
				v += step;
				if(channel != 1)
					lv += step;
				if(channel != 0)
					rv += step;
				xmms_remote_set_volume(cnx.session_id, lv, rv);
				usleep(delay);
			}
		xmms_remote_set_volume(cnx.session_id, olv, orv);
	}

	COM_SYNOPSIS("adjust volume to specified level over time")
	COM_SYNTAX("FADE [LEFT|RIGHT] [target] [stepsize] [delay]")
	COM_DESCRIPTION(
		"The FADE command performs a simple fade effect on the currently playing track.  "
		"If no song is being played, the command has no effect.  Specifying either LEFT "
		"or RIGHT will cause the effect to be isolated to only the specified channel.  "
		"The [target] specifies at which volume the fade effect should end.  The volume "
		"is restored to its previous settings once the target has been reached.  "
		"The [stepsize] "
		"indicates by how much to change the volume at a time.  The [delay] specifies "
		"how long to wait (in microseconds) before decreasing or increasing the volume "
		"towards the target by the specified stepsize.  By default the effect is applied "
		"to both channels, with a target of 0, stepsize of 1, and delay of 100000."
	)
	COM_RETURN("Always 0")
};

static Command *commands[] = {
	new ResetDeviceCommand(),
	new FadeCommand(),
};

void misc_init(void)
{
	for(unsigned i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
		command_add(commands[i]);
}

