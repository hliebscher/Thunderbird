/****************************************************************************
 *
 *   Copyright (c) 2016 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file rc_update.cpp
 *
 * @author Beat Kueng <beat-kueng@gmx.net>
 */

#include "rc_update.h"

using namespace sensors;
using namespace time_literals;

// TODO: find a better home for this
static bool operator ==(const manual_control_switches_s &a, const manual_control_switches_s &b)
{
	return (a.mode_slot == b.mode_slot &&
		a.mode_switch == b.mode_switch &&
		a.return_switch == b.return_switch &&
		a.rattitude_switch == b.rattitude_switch &&
		a.posctl_switch == b.posctl_switch &&
		a.loiter_switch == b.loiter_switch &&
		a.acro_switch == b.acro_switch &&
		a.offboard_switch == b.offboard_switch &&
		a.kill_switch == b.kill_switch &&
		a.arm_switch == b.arm_switch &&
		a.transition_switch == b.transition_switch &&
		a.gear_switch == b.gear_switch &&
		a.stab_switch == b.stab_switch &&
		a.man_switch == b.man_switch);
}

static bool operator !=(const manual_control_switches_s &a, const manual_control_switches_s &b) { return !(a == b); }


RCUpdate::RCUpdate(const Parameters &parameters)
	: _p(parameters)
{
}

void RCUpdate::update_rc_functions()
{
	/* update RC function mappings */
	_rc.function[rc_channels_s::FUNCTION_THROTTLE] = _p.rc_map_throttle - 1;
	_rc.function[rc_channels_s::FUNCTION_ROLL] = _p.rc_map_roll - 1;
	_rc.function[rc_channels_s::FUNCTION_PITCH] = _p.rc_map_pitch - 1;
	_rc.function[rc_channels_s::FUNCTION_YAW] = _p.rc_map_yaw - 1;

	_rc.function[rc_channels_s::FUNCTION_MODE] = _p.rc_map_mode_sw - 1;
	_rc.function[rc_channels_s::FUNCTION_RETURN] = _p.rc_map_return_sw - 1;
	_rc.function[rc_channels_s::FUNCTION_RATTITUDE] = _p.rc_map_rattitude_sw - 1;
	_rc.function[rc_channels_s::FUNCTION_POSCTL] = _p.rc_map_posctl_sw - 1;
	_rc.function[rc_channels_s::FUNCTION_LOITER] = _p.rc_map_loiter_sw - 1;
	_rc.function[rc_channels_s::FUNCTION_ACRO] = _p.rc_map_acro_sw - 1;
	_rc.function[rc_channels_s::FUNCTION_OFFBOARD] = _p.rc_map_offboard_sw - 1;
	_rc.function[rc_channels_s::FUNCTION_KILLSWITCH] = _p.rc_map_kill_sw - 1;
	_rc.function[rc_channels_s::FUNCTION_ARMSWITCH] = _p.rc_map_arm_sw - 1;
	_rc.function[rc_channels_s::FUNCTION_TRANSITION] = _p.rc_map_trans_sw - 1;
	_rc.function[rc_channels_s::FUNCTION_GEAR] = _p.rc_map_gear_sw - 1;
	_rc.function[rc_channels_s::FUNCTION_STAB] = _p.rc_map_stab_sw - 1;
	_rc.function[rc_channels_s::FUNCTION_MAN] = _p.rc_map_man_sw - 1;

	_rc.function[rc_channels_s::FUNCTION_FLAPS] = _p.rc_map_flaps - 1;

	_rc.function[rc_channels_s::FUNCTION_AUX_1] = _p.rc_map_aux1 - 1;
	_rc.function[rc_channels_s::FUNCTION_AUX_2] = _p.rc_map_aux2 - 1;
	_rc.function[rc_channels_s::FUNCTION_AUX_3] = _p.rc_map_aux3 - 1;
	_rc.function[rc_channels_s::FUNCTION_AUX_4] = _p.rc_map_aux4 - 1;
	_rc.function[rc_channels_s::FUNCTION_AUX_5] = _p.rc_map_aux5 - 1;
	_rc.function[rc_channels_s::FUNCTION_AUX_6] = _p.rc_map_aux6 - 1;

	for (int i = 0; i < rc_parameter_map_s::RC_PARAM_MAP_NCHAN; i++) {
		_rc.function[rc_channels_s::FUNCTION_PARAM_1 + i] = _p.rc_map_param[i] - 1;
	}

	/* update the RC low pass filter frequencies */
	_filter_roll.set_cutoff_frequency(_p.rc_flt_smp_rate, _p.rc_flt_cutoff);
	_filter_pitch.set_cutoff_frequency(_p.rc_flt_smp_rate, _p.rc_flt_cutoff);
	_filter_yaw.set_cutoff_frequency(_p.rc_flt_smp_rate, _p.rc_flt_cutoff);
	_filter_throttle.set_cutoff_frequency(_p.rc_flt_smp_rate, _p.rc_flt_cutoff);
	_filter_roll.reset(0.f);
	_filter_pitch.reset(0.f);
	_filter_yaw.reset(0.f);
	_filter_throttle.reset(0.f);
}

void
RCUpdate::rc_parameter_map_poll(ParameterHandles &parameter_handles, bool forced)
{
	if (_rc_parameter_map_sub.updated()) {
		_rc_parameter_map_sub.copy(&_rc_parameter_map);

		/* update parameter handles to which the RC channels are mapped */
		for (int i = 0; i < rc_parameter_map_s::RC_PARAM_MAP_NCHAN; i++) {
			if (_rc.function[rc_channels_s::FUNCTION_PARAM_1 + i] < 0 || !_rc_parameter_map.valid[i]) {
				/* This RC channel is not mapped to a RC-Parameter Channel (e.g. RC_MAP_PARAM1 == 0)
				 * or no request to map this channel to a param has been sent via mavlink
				 */
				continue;
			}

			/* Set the handle by index if the index is set, otherwise use the id */
			if (_rc_parameter_map.param_index[i] >= 0) {
				parameter_handles.rc_param[i] = param_for_used_index((unsigned)_rc_parameter_map.param_index[i]);

			} else {
				parameter_handles.rc_param[i] = param_find(&_rc_parameter_map.param_id[i * (rc_parameter_map_s::PARAM_ID_LEN + 1)]);
			}

		}

		PX4_DEBUG("rc to parameter map updated");

		for (int i = 0; i < rc_parameter_map_s::RC_PARAM_MAP_NCHAN; i++) {
			PX4_DEBUG("\ti %d param_id %s scale %.3f value0 %.3f, min %.3f, max %.3f",
				  i,
				  &_rc_parameter_map.param_id[i * (rc_parameter_map_s::PARAM_ID_LEN + 1)],
				  (double)_rc_parameter_map.scale[i],
				  (double)_rc_parameter_map.value0[i],
				  (double)_rc_parameter_map.value_min[i],
				  (double)_rc_parameter_map.value_max[i]
				 );
		}
	}
}

float
RCUpdate::get_rc_value(uint8_t func, float min_value, float max_value)
{
	if (_rc.function[func] >= 0) {
		float value = _rc.channels[_rc.function[func]];
		return math::constrain(value, min_value, max_value);

	} else {
		return 0.0f;
	}
}

switch_pos_t
RCUpdate::get_rc_sw3pos_position(uint8_t func, float on_th, bool on_inv, float mid_th, bool mid_inv)
{
	if (_rc.function[func] >= 0) {
		float value = 0.5f * _rc.channels[_rc.function[func]] + 0.5f;

		if (on_inv ? value < on_th : value > on_th) {
			return manual_control_switches_s::SWITCH_POS_ON;

		} else if (mid_inv ? value < mid_th : value > mid_th) {
			return manual_control_switches_s::SWITCH_POS_MIDDLE;

		} else {
			return manual_control_switches_s::SWITCH_POS_OFF;
		}

	} else {
		return manual_control_switches_s::SWITCH_POS_NONE;
	}
}

switch_pos_t
RCUpdate::get_rc_sw2pos_position(uint8_t func, float on_th, bool on_inv)
{
	if (_rc.function[func] >= 0) {
		float value = 0.5f * _rc.channels[_rc.function[func]] + 0.5f;

		if (on_inv ? value < on_th : value > on_th) {
			return manual_control_switches_s::SWITCH_POS_ON;

		} else {
			return manual_control_switches_s::SWITCH_POS_OFF;
		}

	} else {
		return manual_control_switches_s::SWITCH_POS_NONE;
	}
}

void
RCUpdate::set_params_from_rc(const ParameterHandles &parameter_handles)
{
	for (int i = 0; i < rc_parameter_map_s::RC_PARAM_MAP_NCHAN; i++) {
		if (_rc.function[rc_channels_s::FUNCTION_PARAM_1 + i] < 0 || !_rc_parameter_map.valid[i]) {
			/* This RC channel is not mapped to a RC-Parameter Channel (e.g. RC_MAP_PARAM1 == 0)
			 * or no request to map this channel to a param has been sent via mavlink
			 */
			continue;
		}

		float rc_val = get_rc_value((rc_channels_s::FUNCTION_PARAM_1 + i), -1.0, 1.0);

		/* Check if the value has changed,
		 * maybe we need to introduce a more aggressive limit here */
		if (rc_val > _param_rc_values[i] + FLT_EPSILON || rc_val < _param_rc_values[i] - FLT_EPSILON) {
			_param_rc_values[i] = rc_val;
			float param_val = math::constrain(
						  _rc_parameter_map.value0[i] + _rc_parameter_map.scale[i] * rc_val,
						  _rc_parameter_map.value_min[i], _rc_parameter_map.value_max[i]);
			param_set(parameter_handles.rc_param[i], &param_val);
		}
	}
}

void
RCUpdate::rc_poll(const ParameterHandles &parameter_handles)
{
	/* read low-level values from FMU or IO RC inputs (PPM, Spektrum, S.Bus) */
	input_rc_s rc_input;

	if (_rc_sub.update(&rc_input)) {

		/* detect RC signal loss */
		bool signal_lost = false;

		/* check flags and require at least four channels to consider the signal valid */
		if (rc_input.rc_lost || rc_input.rc_failsafe || rc_input.channel_count < 4) {
			/* signal is lost or no enough channels */
			signal_lost = true;

		} else {
			/* signal looks good */
			signal_lost = false;

			/* check failsafe */
			int8_t fs_ch = _rc.function[_p.rc_map_failsafe]; // get channel mapped to throttle

			if (_p.rc_map_failsafe > 0) { // if not 0, use channel number instead of rc.function mapping
				fs_ch = _p.rc_map_failsafe - 1;
			}

			if (_p.rc_fails_thr > 0 && fs_ch >= 0) {
				/* failsafe configured */
				if ((_p.rc_fails_thr < _p.min[fs_ch] && rc_input.values[fs_ch] < _p.rc_fails_thr) ||
				    (_p.rc_fails_thr > _p.max[fs_ch] && rc_input.values[fs_ch] > _p.rc_fails_thr)) {
					/* failsafe triggered, signal is lost by receiver */
					signal_lost = true;
				}
			}
		}

		unsigned channel_limit = rc_input.channel_count;

		if (channel_limit > RC_MAX_CHAN_COUNT) {
			channel_limit = RC_MAX_CHAN_COUNT;
		}

		/* read out and scale values from raw message even if signal is invalid */
		for (unsigned int i = 0; i < channel_limit; i++) {

			/*
			 * 1) Constrain to min/max values, as later processing depends on bounds.
			 */
			if (rc_input.values[i] < _p.min[i]) {
				rc_input.values[i] = _p.min[i];
			}

			if (rc_input.values[i] > _p.max[i]) {
				rc_input.values[i] = _p.max[i];
			}

			/*
			 * 2) Scale around the mid point differently for lower and upper range.
			 *
			 * This is necessary as they don't share the same endpoints and slope.
			 *
			 * First normalize to 0..1 range with correct sign (below or above center),
			 * the total range is 2 (-1..1).
			 * If center (trim) == min, scale to 0..1, if center (trim) == max,
			 * scale to -1..0.
			 *
			 * As the min and max bounds were enforced in step 1), division by zero
			 * cannot occur, as for the case of center == min or center == max the if
			 * statement is mutually exclusive with the arithmetic NaN case.
			 *
			 * DO NOT REMOVE OR ALTER STEP 1!
			 */
			if (rc_input.values[i] > (_p.trim[i] + _p.dz[i])) {
				_rc.channels[i] = (rc_input.values[i] - _p.trim[i] - _p.dz[i]) / (float)(_p.max[i] - _p.trim[i] - _p.dz[i]);

			} else if (rc_input.values[i] < (_p.trim[i] - _p.dz[i])) {
				_rc.channels[i] = (rc_input.values[i] - _p.trim[i] + _p.dz[i]) / (float)(_p.trim[i] - _p.min[i] - _p.dz[i]);

			} else {
				/* in the configured dead zone, output zero */
				_rc.channels[i] = 0.0f;
			}

			_rc.channels[i] *= _p.rev[i];

			/* handle any parameter-induced blowups */
			if (!PX4_ISFINITE(_rc.channels[i])) {
				_rc.channels[i] = 0.0f;
			}
		}

		_rc.channel_count = rc_input.channel_count;
		_rc.rssi = rc_input.rssi;
		_rc.signal_lost = signal_lost;
		_rc.timestamp = rc_input.timestamp_last_signal;
		_rc.frame_drop_count = rc_input.rc_lost_frame_count;

		/* publish rc_channels topic even if signal is invalid, for debug */
		_rc_channels_pub.publish(_rc);

		/* only publish manual control if the signal is still present and was present once */
		if (!signal_lost && (rc_input.timestamp_last_signal > 0)) {

			/* initialize manual setpoint */
			manual_control_setpoint_s manual{};

			/* set the timestamp to the last signal time */
			manual.timestamp_last_signal = rc_input.timestamp_last_signal;
			manual.data_source = manual_control_setpoint_s::SOURCE_RC;

			/* limit controls */
			manual.y = get_rc_value(rc_channels_s::FUNCTION_ROLL, -1.0, 1.0);
			manual.x = get_rc_value(rc_channels_s::FUNCTION_PITCH, -1.0, 1.0);
			manual.r = get_rc_value(rc_channels_s::FUNCTION_YAW, -1.0, 1.0);
			manual.z = get_rc_value(rc_channels_s::FUNCTION_THROTTLE, 0.0, 1.0);
			manual.flaps = get_rc_value(rc_channels_s::FUNCTION_FLAPS, -1.0, 1.0);
			manual.aux1 = get_rc_value(rc_channels_s::FUNCTION_AUX_1, -1.0, 1.0);
			manual.aux2 = get_rc_value(rc_channels_s::FUNCTION_AUX_2, -1.0, 1.0);
			manual.aux3 = get_rc_value(rc_channels_s::FUNCTION_AUX_3, -1.0, 1.0);
			manual.aux4 = get_rc_value(rc_channels_s::FUNCTION_AUX_4, -1.0, 1.0);
			manual.aux5 = get_rc_value(rc_channels_s::FUNCTION_AUX_5, -1.0, 1.0);
			manual.aux6 = get_rc_value(rc_channels_s::FUNCTION_AUX_6, -1.0, 1.0);

			/* filter controls */
			manual.y = math::constrain(_filter_roll.apply(manual.y), -1.f, 1.f);
			manual.x = math::constrain(_filter_pitch.apply(manual.x), -1.f, 1.f);
			manual.r = math::constrain(_filter_yaw.apply(manual.r), -1.f, 1.f);
			manual.z = math::constrain(_filter_throttle.apply(manual.z), 0.f, 1.f);

			/* publish manual_control_setpoint topic */
			manual.timestamp = hrt_absolute_time();
			_manual_control_pub.publish(manual);

			// SWITCHES
			manual_control_switches_s sw{};
			sw.mode_slot = manual_control_switches_s::MODE_SLOT_NONE;

			if (_p.rc_map_flightmode > 0) {
				/* number of valid slots */
				const int num_slots = manual_control_switches_s::MODE_SLOT_NUM;

				/* the half width of the range of a slot is the total range
				 * divided by the number of slots, again divided by two
				 */
				const float slot_width_half = 2.0f / num_slots / 2.0f;

				/* min is -1, max is +1, range is 2. We offset below min and max */
				const float slot_min = -1.0f - 0.05f;
				const float slot_max = 1.0f + 0.05f;

				/* the slot gets mapped by first normalizing into a 0..1 interval using min
				 * and max. Then the right slot is obtained by multiplying with the number of
				 * slots. And finally we add half a slot width to ensure that integer rounding
				 * will take us to the correct final index.
				 */
				sw.mode_slot = (((((_rc.channels[_p.rc_map_flightmode - 1] - slot_min) * num_slots) + slot_width_half) /
						 (slot_max - slot_min)) + (1.0f / num_slots)) + 1;

				if (sw.mode_slot > num_slots) {
					sw.mode_slot = num_slots;
				}
			}

			/* mode switches */
			sw.mode_switch = get_rc_sw3pos_position(rc_channels_s::FUNCTION_MODE, _p.rc_auto_th, _p.rc_auto_inv, _p.rc_assist_th,
								_p.rc_assist_inv);
			sw.rattitude_switch = get_rc_sw2pos_position(rc_channels_s::FUNCTION_RATTITUDE, _p.rc_rattitude_th,
					      _p.rc_rattitude_inv);
			sw.posctl_switch = get_rc_sw2pos_position(rc_channels_s::FUNCTION_POSCTL, _p.rc_posctl_th, _p.rc_posctl_inv);
			sw.return_switch = get_rc_sw2pos_position(rc_channels_s::FUNCTION_RETURN, _p.rc_return_th, _p.rc_return_inv);
			sw.loiter_switch = get_rc_sw2pos_position(rc_channels_s::FUNCTION_LOITER, _p.rc_loiter_th, _p.rc_loiter_inv);
			sw.acro_switch = get_rc_sw2pos_position(rc_channels_s::FUNCTION_ACRO, _p.rc_acro_th, _p.rc_acro_inv);
			sw.offboard_switch = get_rc_sw2pos_position(rc_channels_s::FUNCTION_OFFBOARD, _p.rc_offboard_th, _p.rc_offboard_inv);
			sw.kill_switch = get_rc_sw2pos_position(rc_channels_s::FUNCTION_KILLSWITCH, _p.rc_killswitch_th, _p.rc_killswitch_inv);
			sw.arm_switch = get_rc_sw2pos_position(rc_channels_s::FUNCTION_ARMSWITCH, _p.rc_armswitch_th, _p.rc_armswitch_inv);
			sw.transition_switch = get_rc_sw2pos_position(rc_channels_s::FUNCTION_TRANSITION, _p.rc_trans_th, _p.rc_trans_inv);
			sw.gear_switch = get_rc_sw2pos_position(rc_channels_s::FUNCTION_GEAR, _p.rc_gear_th, _p.rc_gear_inv);
			sw.stab_switch = get_rc_sw2pos_position(rc_channels_s::FUNCTION_STAB, _p.rc_stab_th, _p.rc_stab_inv);
			sw.man_switch = get_rc_sw2pos_position(rc_channels_s::FUNCTION_MAN, _p.rc_man_th, _p.rc_man_inv);

			// only publish changes
			if ((sw != _manual_switches) || (hrt_elapsed_time(&_manual_switches.timestamp) >= 1_s)) {
				_manual_switches = sw;
				_manual_switches.timestamp_last_signal = _rc.timestamp;
				_manual_switches.timestamp = hrt_absolute_time();
				_manual_switches_pub.publish(_manual_switches);
			}

			/* copy from mapped manual control to control group 3 */
			actuator_controls_s actuator_group_3{};

			actuator_group_3.timestamp = hrt_absolute_time();
			actuator_group_3.timestamp = rc_input.timestamp_last_signal;

			actuator_group_3.control[0] = manual.y;
			actuator_group_3.control[1] = manual.x;
			actuator_group_3.control[2] = manual.r;
			actuator_group_3.control[3] = manual.z;
			actuator_group_3.control[4] = manual.flaps;
			actuator_group_3.control[5] = manual.aux1;
			actuator_group_3.control[6] = manual.aux2;
			actuator_group_3.control[7] = manual.aux3;

			/* publish actuator_controls_3 topic */
			_actuator_group_3_pub.publish(actuator_group_3);

			/* Update parameters from RC Channels (tuning with RC) if activated */
			if (hrt_elapsed_time(&_last_rc_to_param_map_time) > 1_s) {
				set_params_from_rc(parameter_handles);
				_last_rc_to_param_map_time = hrt_absolute_time();
			}
		}
	}
}
