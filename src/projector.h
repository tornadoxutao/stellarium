/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
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

#ifndef _PROJECTOR_H_
#define _PROJECTOR_H_

#include "stellarium.h"
#include "vecmath.h"
#include "s_font.h"




// Class which handle projection modes and projection matrix
// Overide some function usually handled by glu
class Projector
{
public:
	enum PROJECTOR_TYPE
	{
		PERSPECTIVE_PROJECTOR    = 0,
		FISHEYE_PROJECTOR        = 1,
		CYLINDER_PROJECTOR       = 2,
        STEREOGRAPHIC_PROJECTOR  = 3,
        SPHERIC_MIRROR_PROJECTOR = 4
	};

	enum PROJECTOR_MASK_TYPE
	{
		DISK,
		NONE
	};
	
    static const char *typeToString(PROJECTOR_TYPE type);
    static PROJECTOR_TYPE stringToType(const string &s);

    static const char *maskTypeToString(PROJECTOR_MASK_TYPE type);
    static PROJECTOR_MASK_TYPE stringToMaskType(const string &s);

    
    // Main factory constructor
    static Projector *create(PROJECTOR_TYPE type,
                             const Vec4i& viewport,
                             double _fov = 60.);
	virtual ~Projector();

	virtual PROJECTOR_TYPE getType(void) const {return PERSPECTIVE_PROJECTOR;}
	
	//! Get the type of the mask if any
	PROJECTOR_MASK_TYPE getMaskType(void) const {return maskType;}
	void setMaskType(PROJECTOR_MASK_TYPE m) {maskType = m; }
	
	//! Get and set to define and get viewport size
	virtual void setViewport(int x, int y, int w, int h);
	void setViewport(const Vec4i& v) {setViewport(v[0], v[1], v[2], v[3]);}
	void setViewportPosX(int x) {setViewport(x, vec_viewport[1], vec_viewport[2], vec_viewport[3]);}
	void setViewportPosY(int y) {setViewport(vec_viewport[0], y, vec_viewport[2], vec_viewport[3]);}
	void setViewportWidth(int width) {setViewport(vec_viewport[0], vec_viewport[1], width, vec_viewport[3]);}
	void setViewportHeight(int height) {setViewport(vec_viewport[0], vec_viewport[1], vec_viewport[2], height);}
	int getViewportPosX(void) const {return vec_viewport[0];}
	int getViewportPosY(void) const {return vec_viewport[1];}
	int getViewportWidth(void) const {return vec_viewport[2];}
	int getViewportHeight(void) const {return vec_viewport[3];}
	const Vec4i& getViewport(void) const {return vec_viewport;}
	
	//! Set the current openGL viewport to projector's viewport
	void applyViewport(void) const {glViewport(vec_viewport[0], vec_viewport[1], vec_viewport[2], vec_viewport[3]);}	
	
	//! Set the Field of View in degree
	void set_fov(double f);
	//! Get the Field of View in degree
	double get_fov(void) const {return fov;}
	virtual double getRadPerPixel(void) const
	  {return 1.0/fov*180./M_PI*MY_MIN(getViewportWidth(),getViewportHeight());}

	//! Set the maximum Field of View in degree
	void setMaxFov(double max);
	//! Get the maximum Field of View in degree
	double getMaxFov(void) const {return max_fov;}
    
    //! If is currently zooming, return the target FOV, otherwise return current FOV
    double getAimFov(void) {if (flag_auto_zoom) return zoom_move.aim; else return fov;}
    
    void change_fov(double deltaFov);

	// Update auto_zoom if activated
	void update_auto_zoom(int delta_time);

	// Zoom to the given field of view in degree
	void zoom_to(double aim_fov, float move_duration = 1.);

	// Fill with black around the circle
	void draw_viewport_shape(void);

	void set_clipping_planes(double znear, double zfar);
	void get_clipping_planes(double* zn, double* zf) const {*zn = zNear; *zf = zFar;}

	// Return true if the 2D pos is inside the viewport
	bool check_in_viewport(const Vec3d& pos) const
	{	return 	(pos[1]>vec_viewport[1] && pos[1]<(vec_viewport[1] + vec_viewport[3]) &&
				pos[0]>vec_viewport[0] && pos[0]<(vec_viewport[0] + vec_viewport[2]));}

	// Set the standard modelview matrices used for projection
	void set_modelview_matrices(const Mat4d& _mat_earth_equ_to_eye,
				    const Mat4d& _mat_helio_to_eye,
				    const Mat4d& _mat_local_to_eye,
				    const Mat4d& _mat_j2000_to_eye);

	// Return in vector "win" the projection on the screen of point v in earth equatorial coordinate
	// according to the current modelview and projection matrices (reimplementation of gluProject)
	// Return true if the z screen coordinate is < 1, ie if it isn't behind the observer
	// except for the _check version which return true if the projected point is inside the screen
	inline bool project_earth_equ(const Vec3d& v, Vec3d& win) const
		{return project_custom(v, win, mat_earth_equ_to_eye);}

	inline bool project_earth_equ_check(const Vec3d& v, Vec3d& win) const
		{return project_custom_check(v, win, mat_earth_equ_to_eye);}

	inline bool project_earth_equ_line_check(const Vec3d& v1, Vec3d& win1, const Vec3d& v2, Vec3d& win2) const
		{return project_custom_line_check(v1, win1, v2, win2, mat_earth_equ_to_eye);}

	inline void unproject_earth_equ(double x, double y, Vec3d& v) const
		{unproject(x, y, inv_mat_earth_equ_to_eye, v);}

	// taking account of precession
	inline bool project_j2000(const Vec3d& v, Vec3d& win) const
		{return project_custom(v, win, mat_j2000_to_eye);}

	inline bool project_j2000_check(const Vec3d& v, Vec3d& win) const
		{return project_custom_check(v, win, mat_j2000_to_eye);}

	inline bool project_j2000_line_check(const Vec3d& v1, Vec3d& win1, const Vec3d& v2, Vec3d& win2) const
		{return project_custom_line_check(v1, win1, v2, win2, mat_j2000_to_eye);}

	// Same function with input vector v in heliocentric coordinate
	inline bool project_helio_check(const Vec3d& v, Vec3d& win) const
		{return project_custom_check(v, win, mat_helio_to_eye);}

	inline bool project_helio(const Vec3d& v, Vec3d& win) const
		{return project_custom(v, win, mat_helio_to_eye);}

	inline bool project_helio_line_check(const Vec3d& v1, Vec3d& win1, const Vec3d& v2, Vec3d& win2) const
		{return project_custom_line_check(v1, win1, v2, win2, mat_helio_to_eye);}

	inline void unproject_helio(double x, double y, Vec3d& v) const
		{return unproject(x, y, inv_mat_helio_to_eye, v);}

	// Same function with input vector v in local coordinate
	inline bool project_local(const Vec3d& v, Vec3d& win) const
		{return project_custom(v, win, mat_local_to_eye);}

	inline bool project_local_check(const Vec3d& v, Vec3d& win) const
		{return project_custom_check(v, win, mat_local_to_eye);}

	inline void unproject_local(double x, double y, Vec3d& v) const
		{unproject(x, y, inv_mat_local_to_eye, v);}
		
	// Same function but using a custom modelview matrix
	virtual bool project_custom(const Vec3d& v, Vec3d& win, const Mat4d& mat) const
	{
		gluProject(v[0],v[1],v[2],mat,mat_projection,vec_viewport,&win[0],&win[1],&win[2]);
		return (win[2]<1.);
	}

    bool project_custom_check(const Vec3f& v, Vec3d& win, const Mat4d& mat) const
		{return (project_custom(v, win, mat) && check_in_viewport(win));}
	// project two points and make sure both are in front of viewer and that at least one is on screen

    bool project_custom_line_check(const Vec3f& v1, Vec3d& win1, 
					       const Vec3f& v2, Vec3d& win2, const Mat4d& mat) const
		{return project_custom(v1, win1, mat) && project_custom(v2, win2, mat) && 
		   (check_in_viewport(win1) || check_in_viewport(win2));}


	virtual void unproject_custom(double x, double y, Vec3d& v, const Mat4d& mat) const
	{gluUnProject(x,y,1.,mat,mat_projection,vec_viewport,&v[0],&v[1],&v[2]);}

	// Set the drawing mode in 2D for drawing inside the viewport only.
	// Use reset_perspective_projection() to restore previous projection mode
	void set_orthographic_projection(void) const;

	// Restore the previous projection mode after a call to set_orthographic_projection()
	void reset_perspective_projection(void) const;

	// Reimplementation of gluSphere : glu is overrided for non standard projection
	virtual void sSphere(GLdouble radius, GLdouble one_minus_oblateness,
		GLint slices, GLint stacks,
		const Mat4d& mat, int orient_inside = 0) const;

	// Draw a half sphere
	virtual void sHalfSphere(GLdouble radius, GLint slices, GLint stacks,
		const Mat4d& mat, int orient_inside = 0) const;

	// Draw a disk with a special texturing mode having texture center at center
	virtual void sDisk(GLdouble radius, GLint slices, GLint stacks,
		const Mat4d& mat, int orient_inside = 0) const;	
		
	// Draw a ring with a radial texturing
	virtual void sRing(GLdouble r_min, GLdouble r_max,
	                   GLint slices, GLint stacks,
	                   const Mat4d& mat, int orient_inside) const;

	// Draw a fisheye texture in a sphere
	virtual void sSphere_map(GLdouble radius, GLint slices, GLint stacks,
		const Mat4d& mat, double texture_fov = 2.*M_PI, int orient_inside = 0) const;

	// Reimplementation of gluCylinder : glu is overrided for non standard projection
	virtual void sCylinder(GLdouble radius, GLdouble height, GLint slices, GLint stacks,
		const Mat4d& mat, int orient_inside = 0) const;

	virtual void sVertex3(double x, double y, double z, const Mat4d& mat) const
	{glVertex3d(x,y,z);}

	void print_gravity180(s_font* font, float x, float y, const wstring& str, 
			      bool speed_optimize = 1, float xshift = 0, float yshift = 0) const;
	void print_gravity180(s_font* font, float x, float y, const string& str, 
			      bool speed_optimize = 1, float xshift = 0, float yshift = 0) const
	{
	   	print_gravity180(font, x, y, StelUtility::stringToWstring(str), speed_optimize, xshift, yshift);
	}

	void setFlagGravityLabels(bool gravity) { gravityLabels = gravity; }
	bool getFlagGravityLabels() const { return gravityLabels; }

protected:
	Projector(const Vec4i& viewport, double _fov = 60.);

	// Struct used to store data for auto mov
	typedef struct
	{
		double start;
	    double aim;
	    float speed;
	    float coef;
	}auto_zoom;

	// Init the viewing matrix from the fov, the clipping planes and screen ratio
	// The function is a reimplementation of gluPerspective
	virtual void init_project_matrix(void);

	//! The current projector mask
	PROJECTOR_MASK_TYPE maskType;

	double fov;					// Field of view in degree
	double min_fov;				// Minimum fov in degree
	double max_fov;				// Maximum fov in degree
	double zNear, zFar;			// Near and far clipping planes
	Vec4i vec_viewport;			// Viewport parameters
	Mat4d mat_projection;		// Projection matrix

	Mat4d mat_earth_equ_to_eye;		// Modelview Matrix for earth equatorial projection
	Mat4d mat_j2000_to_eye;         // for precessed equ coords
	Mat4d mat_helio_to_eye;			// Modelview Matrix for earth equatorial projection
	Mat4d mat_local_to_eye;			// Modelview Matrix for earth equatorial projection
	Mat4d inv_mat_earth_equ_to_eye;	// Inverse of mat_projection*mat_earth_equ_to_eye
	Mat4d inv_mat_helio_to_eye;		// Inverse of mat_projection*mat_helio_to_eye
	Mat4d inv_mat_local_to_eye;		// Inverse of mat_projection*mat_local_to_eye
	
	// transformation from screen 2D point x,y to object
	// m is here the already inverted full tranfo matrix
	virtual
    void unproject(double x, double y, const Mat4d& m, Vec3d& v) const
	{
		v.set(	(x - vec_viewport[0]) * 2. / vec_viewport[2] - 1.0,
				(y - vec_viewport[1]) * 2. / vec_viewport[3] - 1.0,
				1.0);
		v.transfo4d(m);
	}
	
	// Automove
	auto_zoom zoom_move;		// Current auto movement
	bool flag_auto_zoom;		// Define if autozoom is on or off
	bool gravityLabels;			// should label text align with the horizon?
};

#endif // _PROJECTOR_H_
