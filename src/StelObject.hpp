/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#ifndef _STEL_OBJECT_BASE_H_
#define _STEL_OBJECT_BASE_H_

#include "vecmath.h"
#include "StelObjectType.hpp"

#include <string>

using namespace std;

class Navigator;
class Projector;
class STexture;

class StelObject;
void intrusive_ptr_add_ref(StelObject* p);
void intrusive_ptr_release(StelObject* p);

class StelObject {
public:
  virtual ~StelObject(void) {}
  
  virtual void retain(void) {;}
  virtual void release(void) {;}

  //! Write I18n information about the object in wstring. 
  virtual wstring getInfoString(const Navigator *nav) const = 0;

  //! The returned wstring can typically be used for object labeling in the sky
  virtual wstring getShortInfoString(const Navigator *nav) const = 0;

  //! Return object's type. It should be the name of the class.
  virtual std::string getType(void) const = 0;

  //! Return object's name
  virtual string getEnglishName(void) const = 0;
  virtual wstring getNameI18n(void) const = 0;

  //! Get position in earth equatorial frame
  virtual Vec3d get_earth_equ_pos(const Navigator *nav) const = 0;

  //! observer centered J2000 coordinates
  virtual Vec3d getObsJ2000Pos(const Navigator *nav) const = 0;

  //! Return object's magnitude
  virtual float get_mag(const Navigator *nav) const {return 99;}
  
  //! Return a priority value which is used to discriminate objects by priority
  //! As for magnitudes, the lower is the higher priority 
  virtual float getSelectPriority(const Navigator *nav) const {return 99;}

  //! Get a color used to display info about the object
  virtual Vec3f getInfoColor(void) const {return Vec3f(1,1,1);}

  //! Return the best FOV in degree to use for a close view of the object
  virtual double get_close_fov(const Navigator *nav) const {return 10.;}

  //! Return the best FOV in degree to use for a global view of the object satellite system (if there are satellites)
  virtual double get_satellites_fov(const Navigator *nav) const {return -1.;}
  virtual double get_parent_satellites_fov(const Navigator *nav) const
    {return -1.;}

  virtual float getOnScreenSize(const Projector *prj, const Navigator *nav = NULL) const {return 0;}
};

#endif
