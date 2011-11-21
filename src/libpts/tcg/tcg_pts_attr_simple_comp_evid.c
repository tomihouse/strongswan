/*
 * Copyright (C) 2011 Sansar Choinyambuu
 * HSR Hochschule fuer Technik Rapperswil
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include "tcg_pts_attr_simple_comp_evid.h"

#include <pa_tnc/pa_tnc_msg.h>
#include <bio/bio_writer.h>
#include <bio/bio_reader.h>
#include <debug.h>

#include <time.h>

typedef struct private_tcg_pts_attr_simple_comp_evid_t private_tcg_pts_attr_simple_comp_evid_t;

/**
 * Simple Component Evidence 
 * see section 3.15.1 of PTS Protocol: Binding to TNC IF-M Specification
 * 
 *					   1				   2				   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Flags      |               Sub-Component Depth             |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                Specific Functional Component                  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                Specific Functional Component                  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | Measure. Type |               Extended into PCR               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |        Hash Algorithm     | PCR Transform |     Reserved      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                    Measurement Date/Time                      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                    Measurement Date/Time                      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                    Measurement Date/Time                      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                    Measurement Date/Time                      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                    Measurement Date/Time                      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |  Optional Policy URI Length   |  Opt. Verification Policy URI ~
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  ~                Optional Verification Policy URI               ~
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Optional PCR Length        |   Optional PCR Before Value   ~
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  ~           Optional PCR Before Value (Variable Length)         ~
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  ~           Optional PCR After Value (Variable Length)          ~
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  ~           Component Measurement (Variable Length)             ~
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

/**
 * Specific Functional Component -> Component Functional Name Structure 
 * see section 5.1 of PTS Protocol: Binding to TNC IF-M Specification
 *
 *					   1				   2				   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Component Functional Name Vendor ID        |Fam| Qualifier |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                   Component Functional Name                   |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */

#define PTS_SIMPLE_COMP_EVID_SIZE					40
#define PTS_SIMPLE_COMP_EVID_MEAS_TIME_SIZE			20
#define PTS_SIMPLE_COMP_EVID_RESERVED				0x00
#define PTS_SIMPLE_COMP_EVID_FAMILY_MASK			0xC0
#define PTS_SIMPLE_COMP_EVID_VALIDATION_MASK		0x60
#define PTS_SIMPLE_COMP_EVID_MEAS_TYPE				(1<<7)
#define PTS_SIMPLE_COMP_EVID_FLAG_PCR				(1<<7)

/**
 * Private data of an tcg_pts_attr_simple_comp_evid_t object.
 */
struct private_tcg_pts_attr_simple_comp_evid_t {

	/**
	 * Public members of tcg_pts_attr_simple_comp_evid_t
	 */
	tcg_pts_attr_simple_comp_evid_t public;

	/**
	 * Attribute vendor ID
	 */
	pen_t vendor_id;

	/**
	 * Attribute type
	 */
	u_int32_t type;

	/**
	 * Attribute value
	 */
	chunk_t value;
	
	/**
	 * Noskip flag
	 */
	bool noskip_flag;
	
	/**
	 * PTS Component Evidence
	 */
	pts_comp_evidence_t *evidence;

};

METHOD(pa_tnc_attr_t, get_vendor_id, pen_t,
	private_tcg_pts_attr_simple_comp_evid_t *this)
{
	return this->vendor_id;
}

METHOD(pa_tnc_attr_t, get_type, u_int32_t,
	private_tcg_pts_attr_simple_comp_evid_t *this)
{
	return this->type;
}

METHOD(pa_tnc_attr_t, get_value, chunk_t,
	private_tcg_pts_attr_simple_comp_evid_t *this)
{
	return this->value;
}

METHOD(pa_tnc_attr_t, get_noskip_flag, bool,
	private_tcg_pts_attr_simple_comp_evid_t *this)
{
	return this->noskip_flag;
}

METHOD(pa_tnc_attr_t, set_noskip_flag,void,
	private_tcg_pts_attr_simple_comp_evid_t *this, bool noskip)
{
	this->noskip_flag = noskip;
}

METHOD(pa_tnc_attr_t, build, void,
	private_tcg_pts_attr_simple_comp_evid_t *this)
{
	bio_writer_t *writer;
	bool has_pcr_info;
	char *utc_time_str, utc_time_buf[25];
	u_int8_t flags;
	u_int32_t depth, extended_pcr;
	pts_comp_func_name_t *name;
	pts_meas_algorithms_t hash_algorithm;
	pts_pcr_transform_t transform;
	pts_comp_evid_validation_t validation;
	time_t measurement_time;
	chunk_t measurement, utc_time, pcr_before, pcr_after, policy_uri;
	
	/* Extract parameters from comp_evidence_t object */
	name         = this->evidence->get_comp_func_name(this->evidence,
							&depth);
	measurement  = this->evidence->get_measurement(this->evidence,
							&extended_pcr, &hash_algorithm, &transform,
							&measurement_time);
	has_pcr_info = this->evidence->get_pcr_info(this->evidence,
							&pcr_before, &pcr_after);
	validation   = this->evidence->get_validation(this->evidence,
							&policy_uri);
	
	/* Determine the flags to set*/
	flags = validation;
	if (has_pcr_info)
	{
		flags |= PTS_SIMPLE_COMP_EVID_FLAG_PCR;
	}

	/* Form the UTC measurement time string */
	if (measurement_time == UNDEFINED_TIME)
	{
		utc_time_str = "0000-00-00T00:00:00Z";
	}
	else
	{
		struct tm t;

		gmtime_r(&measurement_time, &t);
		utc_time_str = utc_time_buf;
		sprintf(utc_time_str, "%04d-%.02d-%.02dT%.02d:%.02d:%.02dZ",
							  t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
							  t.tm_hour, t.tm_min, t.tm_sec);
	}
	utc_time = chunk_create(utc_time_str, PTS_SIMPLE_COMP_EVID_MEAS_TIME_SIZE);	

	writer = bio_writer_create(PTS_SIMPLE_COMP_EVID_SIZE);

	writer->write_uint8 (writer, flags);
	writer->write_uint24(writer, depth);
	writer->write_uint24(writer, name->get_vendor_id(name));
	writer->write_uint8 (writer, name->get_qualifier(name));
	writer->write_uint32(writer, name->get_name(name));
	writer->write_uint8 (writer, PTS_SIMPLE_COMP_EVID_MEAS_TYPE);
	writer->write_uint24(writer, extended_pcr);
	writer->write_uint16(writer, hash_algorithm);
	writer->write_uint8 (writer, transform);
	writer->write_uint8 (writer, PTS_SIMPLE_COMP_EVID_RESERVED);
	writer->write_data  (writer, utc_time);
	
	/* Optional fields */
	if (validation == PTS_COMP_EVID_VALIDATION_FAILED ||
		validation == PTS_COMP_EVID_VALIDATION_PASSED)
	{
		writer->write_uint16(writer, policy_uri.len);
		writer->write_data  (writer, policy_uri);
	}
	if (has_pcr_info)
	{
		writer->write_uint16(writer, pcr_before.len);
		writer->write_data  (writer, pcr_before);
		writer->write_data  (writer, pcr_after);
	}

	writer->write_data(writer, measurement);
	
	this->value = chunk_clone(writer->get_buf(writer));
	writer->destroy(writer);
}

METHOD(pa_tnc_attr_t, process, status_t,
	private_tcg_pts_attr_simple_comp_evid_t *this, u_int32_t *offset)
{
	bio_reader_t *reader;
	pts_comp_func_name_t *name;
	u_int8_t flags, fam_and_qualifier, qualifier, reserved;
	u_int8_t measurement_type, transform, validation;
	u_int16_t hash_algorithm, len;
	u_int32_t depth, vendor_id, comp_name, extended_pcr;
	chunk_t measurement, utc_time, policy_uri, pcr_before, pcr_after;
	time_t measurement_time;
	bool has_pcr_info = FALSE, has_validation = FALSE;

	if (this->value.len < PTS_SIMPLE_COMP_EVID_SIZE)
	{
		DBG1(DBG_TNC, "insufficient data for Simple Component Evidence");
		*offset = 0;
		return FAILED;
	}
	reader = bio_reader_create(this->value);
	
	reader->read_uint8 (reader, &flags);
	reader->read_uint24(reader, &depth);
	reader->read_uint24(reader, &vendor_id);
	reader->read_uint8 (reader, &fam_and_qualifier);
	reader->read_uint32(reader, &comp_name);
	reader->read_uint8 (reader, &measurement_type);
	reader->read_uint24(reader, &extended_pcr);
	reader->read_uint16(reader, &hash_algorithm);
	reader->read_uint8 (reader, &transform);
	reader->read_uint8 (reader, &reserved);
	reader->read_data  (reader, PTS_SIMPLE_COMP_EVID_MEAS_TIME_SIZE, &utc_time);

	if (measurement_type != PTS_SIMPLE_COMP_EVID_MEAS_TYPE)
	{
		DBG1(DBG_TNC, "unsupported Measurement Type in Simple Component Evidence");
		*offset = 12;
		reader->destroy(reader);
		return FAILED;
	}

	validation = flags & PTS_SIMPLE_COMP_EVID_VALIDATION_MASK;
	qualifier = fam_and_qualifier & ~PTS_SIMPLE_COMP_EVID_FAMILY_MASK;

	/* TODO Parse the UTC time string */
	measurement_time = 0;

	/*  Is optional Policy URI field included? */
	if (validation == PTS_COMP_EVID_VALIDATION_FAILED ||
		validation == PTS_COMP_EVID_VALIDATION_PASSED)
	{
		reader->read_uint16(reader, &len);
		reader->read_data(reader, len, &policy_uri);
		has_validation = TRUE;
	}
	
	/*  Are optional PCR value fields included? */
	if (flags & PTS_SIMPLE_COMP_EVID_FLAG_PCR)
	{
		reader->read_uint16(reader, &len);
		reader->read_data(reader, len, &pcr_before);
		reader->read_data(reader, len, &pcr_after);
		has_pcr_info = TRUE;
	}

	/* Measurement field comes at the very end */ 
	reader->read_data(reader,reader->remaining(reader), &measurement);
	reader->destroy(reader);

	/* Create Component Functional Name object */	
	name = pts_comp_func_name_create(vendor_id, comp_name, qualifier);

	/* Create Component Evidence object */
	measurement = chunk_clone(measurement);
	this->evidence = pts_comp_evidence_create(name, depth, extended_pcr,
											  hash_algorithm, transform,
											  measurement_time, measurement);

	/* Add options */
	if (has_validation)
	{
		policy_uri = chunk_clone(policy_uri);
		this->evidence->set_validation(this->evidence, validation, policy_uri);
	}
	if (has_pcr_info)
	{
		pcr_before = chunk_clone(pcr_before);
		pcr_after =  chunk_clone(pcr_after);
		this->evidence->set_pcr_info(this->evidence, pcr_before, pcr_after);
	}

	return SUCCESS;
}

METHOD(pa_tnc_attr_t, destroy, void,
	private_tcg_pts_attr_simple_comp_evid_t *this)
{
	this->evidence->destroy(this->evidence);
	free(this);
}

METHOD(tcg_pts_attr_simple_comp_evid_t, get_comp_evidence, pts_comp_evidence_t*,
	private_tcg_pts_attr_simple_comp_evid_t *this)
{
	return this->evidence;
}

/**
 * Described in header.
 */
pa_tnc_attr_t *tcg_pts_attr_simple_comp_evid_create(pts_comp_evidence_t *evid)
{
	private_tcg_pts_attr_simple_comp_evid_t *this;
	
	INIT(this,
		.public = {
			.pa_tnc_attribute = {
				.get_vendor_id = _get_vendor_id,
				.get_type = _get_type,
				.get_value = _get_value,
				.get_noskip_flag = _get_noskip_flag,
				.set_noskip_flag = _set_noskip_flag,
				.build = _build,
				.process = _process,
				.destroy = _destroy,
			},
			.get_comp_evidence = _get_comp_evidence,
		},
		.vendor_id = PEN_TCG,
		.type = TCG_PTS_SIMPLE_COMP_EVID,
		.evidence = evid,
	);

	return &this->public.pa_tnc_attribute;
}


/**
 * Described in header.
 */
pa_tnc_attr_t *tcg_pts_attr_simple_comp_evid_create_from_data(chunk_t data)
{
	private_tcg_pts_attr_simple_comp_evid_t *this;

	INIT(this,
		.public = {
			.pa_tnc_attribute = {
				.get_vendor_id = _get_vendor_id,
				.get_type = _get_type,
				.get_value = _get_value,
				.get_noskip_flag = _get_noskip_flag,
				.set_noskip_flag = _set_noskip_flag,
				.build = _build,
				.process = _process,
				.destroy = _destroy,
			},
			.get_comp_evidence = _get_comp_evidence,
		},
		.vendor_id = PEN_TCG,
		.type = TCG_PTS_SIMPLE_COMP_EVID,
		.value = chunk_clone(data),
	);

	return &this->public.pa_tnc_attribute;
}
