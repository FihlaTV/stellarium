/*
 * Stellarium Telescope Control Plug-in
 * 
 * Copyright (C) 2006 Johannes Gajdosik
 * Copyright (C) 2009-2011 Bogdan Marinov
 * 
 * This module was originally written by Johannes Gajdosik in 2006
 * as a core module of Stellarium. In 2009 it was significantly extended with
 * GUI features and later split as an external plug-in module by Bogdan Marinov.
 *
 * This class used to be called TelescopeMgr before the split.
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

#ifndef _TELESCOPE_CONTROL_HPP_
#define _TELESCOPE_CONTROL_HPP_

#include "StelFader.hpp"
#include "StelGui.hpp"
#include "StelJsonParser.hpp"
#include "StelObjectModule.hpp"
#include "StelProjectorType.hpp"
#include "StelTextureTypes.hpp"
#include "TelescopeControlGlobals.hpp"
#include "VecMath.hpp"

#include <QFile>
#include <QFont>
#include <QHash>
#include <QMap>
#include <QProcess>
#include <QSettings>
#include <QSignalMapper>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVariant>

class StelNavigator;
class StelObject;
class StelPainter;
class StelProjector;
class TelescopeClient;
class TelescopeControlConfigurationWindow;
class SlewWindow;

using namespace TelescopeControlGlobals;

typedef QSharedPointer<TelescopeClient> TelescopeClientP;

//! This class manages the controlling of one or more telescopes by one
//! instance of the Stellarium program. "Controlling a telescope"
//! means receiving position information from the telescope
//! and sending GOTO commands to the telescope.
//! No esoteric features like motor focus, electric heating and such.
//! The actual controlling of a telescope is left to the implementation
//! of the abstract base class TelescopeClient.
class TelescopeControl : public StelObjectModule
{
	Q_OBJECT

public:
	TelescopeControl();
	virtual ~TelescopeControl();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void deinit();
	virtual void update(double deltaTime);
	virtual void draw(StelCore * core);
	virtual void setStelStyle(const QString& section);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelObjectModule class
	virtual QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const;
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const;
	virtual StelObjectP searchByName(const QString& name) const;
	virtual QStringList listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem=5) const;
	virtual bool configureGui(bool show = true);
	
	///////////////////////////////////////////////////////////////////////////
	// Methods specific to TelescopeControl
	//! Send a J2000-goto-command to the specified telescope
	//! @param telescopeNr the number of the telescope
	//! @param j2000Pos the direction in equatorial J2000 frame
	void telescopeGoto(int telescopeNr, const Vec3d &j2000Pos);
	
	//! Remove all currently registered telescopes
	void deleteAllTelescopes();
	
	//! Safe access to the loaded list of telescope models
	const QHash<QString, DeviceModel>& getDeviceModels();
	//! \todo Add description
	const QHash<QString, QString>& getIndiDeviceModels();
	
	//! Loads the module's configuration from the configuration file.
	void loadConfiguration();
	//! Saves the module's configuration to the configuration file.
	void saveConfiguration();
	
	//! Saves to telescopes.json a list of the parameters of the active telescope clients.
	void saveTelescopes();
	//! Loads from telescopes.json the parameters of telescope clients and initializes them. If there are already any initialized telescope clients, they are removed.
	void loadTelescopes();
	
	//These are public, but not slots, because they don't use sufficient validation. Scripts shouldn't be able to add/remove telescopes, only to point them.
	//! Adds a telescope description containing the given properties. DOES NOT VALIDATE its parameters. If serverName is specified, portSerial should be specified too. Call saveTelescopes() to write the modified configuration to disc. Call startTelescopeAtSlot() to start this telescope.
	//! @param portSerial must be a valid serial port name for the particular platform, e.g. "COM1" for Microsoft Windows of "/dev/ttyS0" for Linux
	//! TODO: Update description.
	bool addTelescopeAtSlot(int slot, const QVariantMap& properties);
	//! Retrieves a telescope description.
	//! \returns empty map if there is nothing at that slot.
	const QVariantMap getTelescopeAtSlot(int slot) const;
	//! Removes info from the tree. Should it include stopTelescopeAtSlot()?
	bool removeTelescopeAtSlot(int slot);
	
	//! Starts a telescope at the given slot, getting its description with getTelescopeAtSlot(). Creates a TelescopeClient object and starts a server process if necessary.
	bool startTelescopeAtSlot(int slot);
	//! Stops the telescope at the given slot. Destroys the TelescopeClient object and terminates the server process if necessary.
	bool stopTelescopeAtSlot(int slot);
	//! Stops all telescopes, but without removing them like deleteAllTelescopes().
	bool stopAllTelescopes();
	
	//! Checks if there's a TelescopeClient object at a given slot, i.e. if there's an active telescope at that slot.
	bool isExistingClientAtSlot(int slot);
	//! Checks if the TelescopeClient object at a given slot is connected to a server.
	bool isConnectedClientAtSlot(int slot);

	//! Returns a list of the currently connected clients
	QHash<int, QString> getConnectedClientsNames();
	
	bool getFlagUseTelescopeServerLogs () {return useTelescopeServerLogs;}

#ifdef Q_OS_WIN32
	//! \returns true if the ASCOM platform has been detected.
	bool canUseAscom() const {return ascomPlatformIsInstalled;}
#endif

public slots:
	//! Set display flag for telescope reticles
	void setFlagTelescopeReticles(bool b) {reticleFader = b;}
	//! Get display flag for telescope reticles
	bool getFlagTelescopeReticles() const {return (bool)reticleFader;}
	
	//! Set display flag for telescope name labels
	void setFlagTelescopeLabels(bool b) {labelFader = b;}
	//! Get display flag for telescope name labels
	bool getFlagTelescopeLabels() const {return labelFader==true;}

	//! Set display flag for telescope field of view circles
	void setFlagTelescopeCircles(bool b) {circleFader = b;}
	//! Get display flag for telescope field of view circles
	bool getFlagTelescopeCircles() const {return circleFader==true;}
	
	//! Set the telescope reticle color
	void setReticleColor(const Vec3f &c) {reticleColor = c;}
	//! Get the telescope reticle color
	const Vec3f& getReticleColor() const {return reticleColor;}
	
	//! Get the telescope labels color
	const Vec3f& getLabelColor() const {return labelColor;}
	//! Set the telescope labels color
	void setLabelColor(const Vec3f &c) {labelColor = c;}

	//! Set the field of view circles color
	void setCircleColor(const Vec3f &c) {circleColor = c;}
	//! Get the field of view circles color
	const Vec3f& getCircleColor() const {return circleColor;}
	
	//! Define font size to use for telescope names display
	void setFontSize(int fontSize);
	
	//! slews a telescope to the selected object.
	//! For use from the GUI. The telescope number will be
	//! deduced from the name of the QAction which triggered the slot.
	void slewTelescopeToSelectedObject(int number);

	//! slews a telescope to the point of the celestial sphere currently
	//! in the center of the screen.
	//! For use from the GUI. The telescope number will be
	//! deduced from the name of the QAction which triggered the slot.
	void slewTelescopeToViewDirection(int number);
	
	//! Used in the GUI
	void setFlagUseTelescopeServerLogs (bool b) {useTelescopeServerLogs = b;}

signals:
	void clientConnected(int slot, QString name);
	void clientDisconnected(int slot);
	
private:
	//! Draw a nice animated pointer around the object if it's selected
	void drawPointer(const StelProjectorP& prj, const StelNavigator* nav, StelPainter& sPainter);

	//! Perform the communication with the telescope servers
	void communicate();

	//! Returns the path to the "modules/TelescopeControl" directory.
	//! Returns an empty string if it doesn't exist.
	QString getPluginDirectoryPath() const;
	//! Returns the path to the "connections.json" file
	QString getConnectionsFilePath() const;
	
	LinearFader labelFader;
	LinearFader reticleFader;
	LinearFader circleFader;
	//! Colour currently used to draw telescope reticles
	Vec3f reticleColor;
	//! Colour currently used to draw telescope text labels
	Vec3f labelColor;
	//! Colour currently used to draw field of view circles
	Vec3f circleColor;
	//! Colour used to draw telescope reticles in normal mode, as set in the configuration file
	Vec3f reticleNormalColor;
	//! Colour used to draw telescope reticles in night mode, as set in the configuration file
	Vec3f reticleNightColor;
	//! Colour used to draw telescope labels in normal mode, as set in the configuration file
	Vec3f labelNormalColor;
	//! Colour used to draw telescope labels in night mode, as set in the configuration file
	Vec3f labelNightColor;
	//! Colour used to draw field of view circles in normal mode, as set in the configuration file
	Vec3f circleNormalColor;
	//! Colour used to draw field of view circles in night mode, as set in the configuration file
	Vec3f circleNightColor;
	
	//! Font used to draw telescope text labels
	QFont labelFont;
	
	//Toolbar button to toggle the Slew window
	QPixmap* pixmapHover;
	QPixmap* pixmapOnIcon;
	QPixmap* pixmapOffIcon;
	StelButton* toolbarButton;
	
	//! Telescope reticle texture
	StelTextureSP reticleTexture;
	//! Telescope selection marker texture
	StelTextureSP selectionTexture;
	
	//! Contains the initialized telescope client objects representing the telescopes that Stellarium is connected to or attempting to connect to.
	QMap<int, TelescopeClientP> telescopeClients;


	QStringList telescopeServers;
	QVariantMap telescopeDescriptions;
	QHash<QString, DeviceModel> deviceModels;
	//! \todo Temporary.
	QHash<QString, QString> indiDeviceModels;
	
	QStringList interfaceTypeNames;
	
	bool useTelescopeServerLogs;
	QHash<int, QFile*> telescopeServerLogFiles;
	QHash<int, QTextStream*> telescopeServerLogStreams;
	
	//GUI
	TelescopeControlConfigurationWindow* configurationWindow;
	SlewWindow* slewWindow;
	
	//! Checks if the argument is a valid slot number. Used internally.
	bool isValidSlotNumber(int slot) const;
	//! Checks if the argument is a TCP port number in IANA's allowed range.
	bool isValidPort(uint port);
	//! Checks if the argument is a valid delay value in microseconds.
	bool isValidDelay(int delay);

	QSignalMapper gotoSelectedShortcutMapper;
	QSignalMapper gotoDirectionShortcutMapper;

	//! A wrapper for TelescopeControl::createClient(). Used internally by
	//! loadTelescopes() and startTelescopeAtSlot().
	//! Does not perform any validation on its arguments.
	//! It is separate because the previous implementation separated clients
	//! from servers.
	//! TODO: Do away with this?
	bool startClientAtSlot(int slot, const QVariantMap& properties);

	//! Creates a client object belonging to a subclass of TelescopeClient.
	//! Replaces TelescopeClient::create().
	//! \returns a base class pointer to the client object.
	TelescopeClient* createClient(const QVariantMap& properties);
	
	//! Returns true if the client has been stopped successfully or doesn't exist.
	bool stopClientAtSlot(int slot);
	

	//! Loads the list of supported telescope models.
	void loadDeviceModels();
	//! If the INDI library is installed, loads the list of available INDI
	//! drivers
	void loadIndiDeviceModels();
	
	//! Copies the default device_models.json to the given destination
	//! \returns true if the file has been copied successfully
	bool restoreDeviceModelsListTo(QString deviceModelsListPath);
	
	void addLogAtSlot(int slot);
	void logAtSlot(int slot);
	void removeLogAtSlot(int slot);

#ifdef Q_OS_WIN32
	bool ascomPlatformIsInstalled;
	bool checkIfAscomIsInstalled();
#endif
};


#include "fixx11h.h"
#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class TelescopeControlStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif /*_TELESCOPE_CONTROL_HPP_*/
