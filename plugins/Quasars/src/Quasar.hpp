/*
 * Copyright (C) 2011 Alexander Wolf
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _QUASAR_HPP_
#define _QUASAR_HPP_ 1

#include <QVariant>
#include <QString>
#include <QStringList>
#include <QFont>
#include <QList>
#include <QDateTime>

#include "StelObject.hpp"
#include "StelTextureTypes.hpp"
#include "StelPainter.hpp"
#include "StelFader.hpp"

class StelPainter;

//! @class Quasar
//! A Quasar object represents one Quasar on the sky.
//! Details about the Quasars are passed using a QVariant which contains
//! a map of data from the json file.

class Quasar : public StelObject
{
	friend class Quasars;
public:
	//! @param id The official designation for a quasar, e.g. "RXS J00066+4342"
	Quasar(const QVariantMap& map);
	~Quasar();

	//! Get a QVariantMap which describes the Quasar.  Could be used to
	//! create a duplicate.
	QVariantMap getMap(void);

	virtual QString getType(void) const
	{
		return "Quasar";
	}
	virtual float getSelectPriority(const StelCore* core) const;

	//! Get an HTML string to describe the object
	//! @param core A pointer to the core
	//! @flags a set of flags with information types to include.
	virtual QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const;
	virtual Vec3f getInfoColor(void) const;
	virtual Vec3d getJ2000EquatorialPos(const StelCore*) const
	{
		return XYZ;
	}
        virtual float getVMagnitude(const StelCore* core, bool withExtinction=false) const;
	virtual double getAngularSize(const StelCore* core) const;
	virtual QString getNameI18n(void) const
	{
		return designation;
	}
	virtual QString getEnglishName(void) const
	{
		return designation;
	}

	void update(double deltaTime);

private:
	bool initialized;

	Vec3d XYZ;                         // holds J2000 position

	static StelTextureSP hintTexture;

	void draw(StelCore* core, StelPainter& painter);

	// Quasar
	QString designation;		//! The ID of the quasar
	float VMagnitude;		//! Visual magnitude
	float AMagnitude;		//! Absolute magnitude
	float bV;			//! B-V color index
	double qRA;			//! R.A. J2000 for the quasar
	double qDE;			//! Dec. J2000 for the quasar	
	float redshift;			//! Distance to quasar (redshift)

	LinearFader labelsFader;
};

#endif // _SUPERNOVA_HPP_
