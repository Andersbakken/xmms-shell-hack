#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xmmsctrl.h>
#include <cctype>
#include "config.h"
#include "command.h"

#define SECTION	virtual const string get_section(void) const { return "Volume Control"; }

class VolumeCommand : public Command
{
public:
	COM_STRUCT(VolumeCommand, "volume")

	virtual void execute(CommandContext &context) const
	{
        Session session = context.session;
		bool setv = false;
		int chan = 0;
		gint32 lv, rv, v;

        session.get_volume(lv, rv);
		v = (lv + rv) / 2;
		if(context.args.size() > 1) {
			if(isdigit(context.args[1][0])) {
				v = atoi(context.args[1].c_str());
				setv = true;
			} else {
				if(string("left").substr(0, context.args[1].length()) == context.args[1])
					chan = 1;
				else if(string("right").substr(0, context.args[1].length()) == context.args[1])
					chan = 2;
				else {
					context.result_code = COMERR_SYNTAX;
					return;
				}
				if(context.args.size() > 2) {
					setv = true;
					if(!isdigit(context.args[2][0])) {
						context.result_code = COMERR_SYNTAX;
						return;
					}
					v = atoi(context.args[2].c_str());
				}
			}
		}
		if(setv) {
			if(v > 100)
				v = 100;
			switch(chan) {
				case 0:
                    session.set_volume(v, v);
					printf("The volume of both channels has been set to %d\n", v);
					break;
				case 1:
                    session.set_volume(v, rv);
					printf("The volume of the left channel has been set to %d\n", v);
					break;
				case 2:
                    session.set_volume(lv, v);
					printf("The volume of the right channel has been set to %d\n", v);
					break;
			}
			context.result_code = v;
		} else {
			switch(chan) {
				case 0:
					printf("Left channel volume: %d\nRight channel volume: %d\n", lv, rv);
					break;
				case 1:
					printf("Left channel volume: %d\n", lv);
					v = lv;
					break;
				case 2:
					printf("Right channel volume: %d\n", rv);
					v = rv;
					break;
			}
			context.result_code = v;
		}
	}

	COM_SYNOPSIS("set the volume in either or both channels to a given level")
	COM_SYNTAX("VOLUME [left|right] [<value>]")
	COM_DESCRIPTION(
		"The VOLUME command queries or sets the volume level of XMMS. If the <value> "
		"argument is given, then the volume will be set.  If not, a volume will be displayed "
		"and returned.  If a volume is given, it should be specified as an integer from 0 through "
		"100 inclusive (any given value will be coerced into this range).  If [left] or [right] is "
		"given, only that channel is operated upon.  That is, if a channel is specified and no volume "
		"is given, the current volume setting for that channel is displayed and returned.  If a volume "
		"and channel are specified, the volume will only be applied to the given channel.  If a volume "
		"is given and no channel is specified, both channels will be set to the given volume."
	)
	COM_RETURN(
		"If no parameters are given, returns the average of the volumes in the two channels.  "
		"If a value is given, that value is returned.  "
		"Otherwise, when the left or right channel is specified, returns the volume of that channel.  "
	)
	SECTION
};

class VolumeControlCommand : public Command
{
	int mod;
public:
	VolumeControlCommand(const string &_primary_name, int _mod) : Command(_primary_name), mod(_mod) { }
	virtual ~VolumeControlCommand() { }

	virtual void execute(CommandContext &cnx) const
	{
        Session session = cnx.session;
		gint32 lv, rv, v = 2, chan = 0;

        session.get_volume(lv, rv);
		if(cnx.args.size() > 1) {
			if(isdigit(cnx.args[1][0]))
				v = atoi(cnx.args[1].c_str());
			else {
				if(string("left").substr(0, cnx.args[1].length()) == cnx.args[1])
					chan = 1;
				else if(string("right").substr(0, cnx.args[1].length()) == cnx.args[1])
					chan = 2;
				else {
					cnx.result_code = COMERR_SYNTAX;
					return;
				}
				if(cnx.args.size() > 2) {
					if(isdigit(cnx.args[2][0]))
						v = atoi(cnx.args[2].c_str());
					else {
						cnx.result_code = COMERR_SYNTAX;
						return;
					}
				}
			}
		}
		switch(chan) {
			case 0:
				lv += mod * v;
				rv += mod * v;
				printf("The volume of the left channel has been set to %d\n", lv);
				printf("The volume of the right channel has been set to %d\n", rv);
				cnx.result_code = (lv + rv) / 2;
				break;
			case 1:
				cnx.result_code = (lv += mod * v);
				printf("The volume of the left channel has been set to %d\n", lv);
				break;
			case 2:
				cnx.result_code = (rv += mod * v);
				printf("The volume of the right channel has been set to %d\n", rv);
				break;
		}
		if(lv < 0)
			lv = 0;
		if(rv < 0)
			rv = 0;
		if(lv > 100)
			lv = 100;
		if(rv > 100)
			rv = 100;
        session.set_volume(lv, rv);
	}

	SECTION
};

class VolumeUpCommand : public VolumeControlCommand
{
public:
	VolumeUpCommand(void) : VolumeControlCommand("upvolume", 1) { add_alias("+"); }
	virtual ~VolumeUpCommand() { }

	COM_SYNOPSIS("increase the volume in either or both channels")
	COM_SYNTAX("UPVOLUME [left|right] [<step>]")
	COM_DESCRIPTION(
			"If <step> is not given, it assumes a default value of 2.  If no channel is specified, "
			"the volume of both channels is increased by <step>.  Otherwise the volume increase "
			"is applied only to the given channel."
	)
	COM_RETURN(
		"If no channel is specified, returns the average of the new volumes in the two channels.  "
		"Otherwise, when the left or right channel is specified, returns the new volume of that channel."
	)
};

class VolumeDownCommand : public VolumeControlCommand
{
public:
	VolumeDownCommand(void) : VolumeControlCommand("downvolume", -1) { add_alias("-"); }
	virtual ~VolumeDownCommand() { }

	COM_SYNOPSIS("decrease the volume in either or both channels")
	COM_SYNTAX("DOWNVOLUME [left|right] [<step>]")
	COM_DESCRIPTION(
			"If <step> is not given, it assumes a default value of 2.  If no channel is specified, "
			"the volume of both channels is decreased by <step>.  Otherwise the volume decrease "
			"is applied only to the given channel."
	)
	COM_RETURN(
		"If no channel is specified, returns the average of the new volumes in the two channels.  "
		"Otherwise, when the left or right channel is specified, returns the new volume of that channel."
	)
};

class BalanceCommand : public Command
{
public:
	COM_STRUCT(BalanceCommand, "balance")

	virtual void execute(CommandContext &context) const
	{
        Session session = context.session;
		int bal;

		if(context.args.size() > 1) {
			if(isdigit(context.args[1][0]) ||
			   ((context.args[1][0] == '-' || context.args[1][0] == '+') &&
			     isdigit(context.args[1][1]))) {
				bal = atoi(context.args[1].c_str());
                session.set_balance(bal);
				printf("Balance level is now %d\n", bal);
				context.result_code = bal;
				return;
			}
			context.result_code = COMERR_SYNTAX;
			return;
		}
        bal = session.get_balance();
		printf("Balance level is %d\n", bal);
		context.result_code = bal;
	}

	COM_SYNOPSIS("set or query balance level")
	COM_SYNTAX("BALANCE <value>")
	COM_DESCRIPTION(
		"If <value> is given, the balance is set to that value.  The valid range is "
		"-100 through 100 inclusive (any given value will be coerced into this range).  "
		"A value of 0 specifies the same volume for both the left and right channels, "
		"while negative values imply a louder left channel and positive values "
		"imply a louder right channel.  If no <value> is specified, the current balance "
		"setting is displayed."
	)
	COM_RETURN(
		"If a value is specified, that value is returned.  Otherwise the current "
		"balance level is returned."
	)

	SECTION
};

#if HAVE_XMMS_REMOTE_GET_EQ_PREAMP
class PreampCommand : public Command
{
public:
	COM_STRUCT(PreampCommand, "preamp")
	
	virtual void execute(CommandContext &cnx) const
	{
        Session session = cnx.session;
		float v;

		cnx.result_code = 0;
		if(cnx.args.size() < 2) {
			printf("Preamp: %.1f\n", session.get_eq_preamp());
			return;
		}
		if(!isdigit(cnx.args[1][0]) && (cnx.args[1][0] != '-' || !isdigit(cnx.args[1][1]))) {
			cnx.result_code = COMERR_SYNTAX;
			return;
		}
		v = atof(cnx.args[1].c_str());
        session.set_eq_preamp(v);
		usleep(10);
        v = session.get_eq_preamp();
		printf("Preamp set to: %.1f\n", v);
	}

	COM_SYNOPSIS("view or set the equalizer's preamp value")
	COM_SYNTAX("PREAMP [value]")
	COM_DESCRIPTION(
		"The PREAMP command will output the equalizer's preamp value if no arguments "
		"are given.  Otherwise its argument, which should be a decimal value between "
		"-20 and 20, is assigned."
	)
	COM_RETURN("Always 0")
	SECTION
};
#endif

#if HAVE_XMMS_REMOTE_GET_EQ_BAND
class BandCommand : public Command
{
public:
	COM_STRUCT(BandCommand, "band")
	
	virtual void execute(CommandContext &cnx) const
	{
        Session session = cnx.session;
		int band;
		vector<float> values;
        float v;

		cnx.result_code = 0;
		if(cnx.args.size() < 2) {
            session.get_eq(v, values);
			for(band = 0; band < 10; band++)
				printf("Band %d: %.1f\n", band, values[band]);
			return;
		}
		if(!isdigit(cnx.args[1][0]) || (band = atoi(cnx.args[1].c_str())) < 0 || band > 9) {
			cnx.result_code = COMERR_SYNTAX;
			return;
		}
		if(cnx.args.size() < 3) {
			printf("Band %d: %.1f\n", band, session.get_eq_band(band));
			return;
		}
		if(!isdigit(cnx.args[2][0]) && (cnx.args[2][0] != '-' || !isdigit(cnx.args[2][1]))) {
			cnx.result_code = COMERR_SYNTAX;
			return;
		}
		v = atof(cnx.args[2].c_str());
        session.set_eq_band(band, v);
		usleep(10);
        v = session.get_eq_band(band);
		printf("Band %d set to: %.1f\n", band, v);
	}
	
	COM_SYNOPSIS("view or set the value of equalizer bands")
	COM_SYNTAX("BAND [band] [value]")
	COM_DESCRIPTION(
		"The BAND command outputs or sets the value of an equalizer band.  "
		"If given no arguments, the BAND command outputs the current values "
		"of all 10 equalizer bands.  If only a band value is specified "
		"(a value from 0 through 9) then the value of that particular band "
		"is output.  Otherwise the second argument, which should be a decimal "
		"value between -20 and 20, is assigned to the given band."
	)
	COM_RETURN("Always 0")
	SECTION
};
#endif

static Command *commands[] = {
	new VolumeCommand(),
	new VolumeUpCommand(),
	new VolumeDownCommand(),
	new BalanceCommand(),
#if HAVE_XMMS_REMOTE_GET_EQ_PREAMP
	new PreampCommand(),
#endif
#if HAVE_XMMS_REMOTE_GET_EQ_BAND
	new BandCommand(),
#endif
};

void volume_init(void)
{
	for(unsigned i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
		command_add(commands[i]);
}

