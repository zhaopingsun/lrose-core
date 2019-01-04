// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=* 
// ** Copyright UCAR (c) 1990 - 2016                                         
// ** University Corporation for Atmospheric Research (UCAR)                 
// ** National Center for Atmospheric Research (NCAR)                        
// ** Boulder, Colorado, USA                                                 
// ** BSD licence applies - redistribution and use in source and binary      
// ** forms, with or without modification, are permitted provided that       
// ** the following conditions are met:                                      
// ** 1) If the software is modified to produce derivative works,            
// ** such modified software should be clearly marked, so as not             
// ** to confuse it with the version available from UCAR.                    
// ** 2) Redistributions of source code must retain the above copyright      
// ** notice, this list of conditions and the following disclaimer.          
// ** 3) Redistributions in binary form must reproduce the above copyright   
// ** notice, this list of conditions and the following disclaimer in the    
// ** documentation and/or other materials provided with the distribution.   
// ** 4) Neither the name of UCAR nor the names of its contributors,         
// ** if any, may be used to endorse or promote products derived from        
// ** this software without specific prior written permission.               
// ** DISCLAIMER: THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS  
// ** OR IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED      
// ** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.    
// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=* 
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

// RCS info
//   $Author: jcraig $
//   $Locker:  $
//   $Date: 2018/01/26 20:36:36 $
//   $Id: Chill2netCDFSweep.cc,v 1.11 2018/01/26 20:36:36 jcraig Exp $
//   $Revision: 1.11 $
//   $State: Exp $
//
 
/**-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-**/
/*********************************************************************
 * Chill2netCDFSweep: Chill2netCDFSweep program object.
 *
 * RAP, NCAR, Boulder CO
 *
 * July 2005
 *
 * Nancy Rehak
 *
 *********************************************************************/

#include <cassert>
#include <iostream>
#include <csignal>
#include <cmath>
#include <string>

#include <dsdata/DsFileListTrigger.hh>
#include <dsdata/DsInputDirTrigger.hh>
#include <toolsa/pmu.h>
#include <toolsa/umisc.h>

#include "Cinrad2netCDFSweep.hh"
#include "SweepFile.hh"
#include "rdats.h"

using namespace std;

// Global variables

// Calculate the constant used to convert the angle values in the 
// input files to degrees.  This is a conversion of a 16 bit angle
// to degrees.

const int Chill2netCDFSweep::INIT_NUM_GATES = 500;
const int Chill2netCDFSweep::INIT_NUM_BEAMS = 360;

const double Chill2netCDFSweep::ANGLE_TO_DEGREE = 360.0 / 65536.0;

const int Chill2netCDFSweep::PARAM_AVGI_MASK = 1;
const int Chill2netCDFSweep::PARAM_AVGQ_MASK = 2;
const int Chill2netCDFSweep::PARAM_DBZ_MASK = 4;
const int Chill2netCDFSweep::PARAM_VEL_MASK = 8;
const int Chill2netCDFSweep::PARAM_NCP_MASK = 16;
const int Chill2netCDFSweep::PARAM_SW_MASK = 32;

const int Chill2netCDFSweep::CHANNEL_H_MASK = 1;
const int Chill2netCDFSweep::CHANNEL_V_MASK = 2;

const float Chill2netCDFSweep::RADAR_C = 287.69;
const float Chill2netCDFSweep::ANT_GAIN_H = 42.2;
const float Chill2netCDFSweep::ANT_GAIN_V = 42.2;
const float Chill2netCDFSweep::REC_GAIN_H = 116.3;
const float Chill2netCDFSweep::REC_GAIN_V = 116.3;
const float Chill2netCDFSweep::TX_POW_H = 88.5;
const float Chill2netCDFSweep::TX_POW_V = 87.0;


Chill2netCDFSweep *Chill2netCDFSweep::_instance =
     (Chill2netCDFSweep *)NULL;


/*********************************************************************
 * Constructor
 */

Chill2netCDFSweep::Chill2netCDFSweep(int argc, char **argv) :
  _sweepStartTime(0),
  _startElevation(0),
  _sweepNumber(0),
  _startRangeMm(0),
  _gateSpacingMm(0),
  _numGatesInSweep(0),
  _samplesPerBeam(0),
  _nyquistVelocity(0.0),
  _prt(0),
  _azimuth(0),
  _elevation(0),
  _timeOffset(0),
  _hAvgI(0),
  _hAvgQ(0),
  _hDbz(0),
  _hVel(0),
  _hNcp(0),
  _hSw(0),
  _hNiq(0),
  _hAiq(0),
  _hDm(0),
  _vAvgI(0),
  _vAvgQ(0),
  _vDbz(0),
  _vVel(0),
  _vNcp(0),
  _vSw(0),
  _vNiq(0),
  _vAiq(0),
  _vDm(0),
  _beamIndex(0),
  _numGatesAlloc(0),
  _numBeamsAlloc(0)
{
  static const string method_name = "Chill2netCDFSweep::Chill2netCDFSweep()";
  
  // Make sure the singleton wasn't already created.

  assert(_instance == (Chill2netCDFSweep *)NULL);
  
  // Initialize the okay flag.

  okay = true;
  
  // Set the singleton instance pointer

  _instance = this;

  // Set the program name.

  path_parts_t progname_parts;
  
  uparse_path(argv[0], &progname_parts);
  _progName = STRdup(progname_parts.base);
  
  // Display ucopyright message.

  ucopyright(_progName);

  // Get the command line arguments.

  _args = new Args(argc, argv, (char *) "Chill2netCDFSweep");

  // Get TDRP parameters.

  _params = new Params();
  char *params_path = (char *) "unknown";
  
  if (_params->loadFromArgs(argc, argv,
			    _args->override.list,
			    &params_path))
  {
    cerr << "ERROR: " << method_name << endl;
    cerr << "Problem with TDRP parameters in file: " << params_path << endl;
    
    okay = false;
    
    return;
  }
}


/*********************************************************************
 * Destructor
 */

Chill2netCDFSweep::~Chill2netCDFSweep()
{
  // Free contained objects

  delete _params;
  delete _args;
  delete _dataTrigger;
  
  // Free the data arrays

  delete [] _azimuth;
  delete [] _elevation;
  delete [] _timeOffset;

  delete [] _hAvgI;
  delete [] _hAvgQ;
  delete [] _hDbz;
  delete [] _hVel;
  delete [] _hNcp;
  delete [] _hSw;
  delete [] _hNiq;
  delete [] _hAiq;
  delete [] _hDm;
  
  delete [] _vAvgI;
  delete [] _vAvgQ;
  delete [] _vDbz;
  delete [] _vVel;
  delete [] _vNcp;
  delete [] _vSw;
  delete [] _vNiq;
  delete [] _vAiq;
  delete [] _vDm;
}


/*********************************************************************
 * Inst() - Retrieve the singleton instance of this class.
 */

Chill2netCDFSweep *Chill2netCDFSweep::Inst(int argc, char **argv)
{
  if (_instance == (Chill2netCDFSweep *)NULL)
    new Chill2netCDFSweep(argc, argv);
  
  return(_instance);
}

Chill2netCDFSweep *Chill2netCDFSweep::Inst()
{
  assert(_instance != (Chill2netCDFSweep *)NULL);
  
  return(_instance);
}


/*********************************************************************
 * init() - Initialize the local data.
 *
 * Returns true if the initialization was successful, false otherwise.
 */

bool Chill2netCDFSweep::init()
{
  static const string method_name = "Chill2netCDFSweep::init()";
  
  // Initialize the data trigger

  switch (_params->trigger_mode)
  {
  case Params::FILE_LIST :
  {
    DsFileListTrigger *trigger = new DsFileListTrigger();
    if (trigger->init(_args->getInputFileList()) != 0)
    {
      cerr << "ERROR: " << method_name << endl;
      cerr << "Error initializing FILE_LIST trigger" << endl;
      
      return false;
    }
    
    _dataTrigger = trigger;
    
    break;
  }
  
  case Params::INPUT_DIR :
  {
    DsInputDirTrigger *trigger = new DsInputDirTrigger();
    if (trigger->init(_params->input_dir,
		      "", false,
		      PMU_auto_register) != 0)
    {
      cerr << "ERROR: " << method_name << endl;
      cerr << "Error initializing INPUT_DIR trigger" << endl;
      
      return false;
    }
    
    _dataTrigger = trigger;
    
    break;
  }
  
  } /* endswitch - _params->trigger_mode */
  
  // Allocate a fair size for the data arrays initially.  The array
  // sizes will be increased as needed.

  _allocateDataArrays(INIT_NUM_GATES, INIT_NUM_BEAMS);
  
  // initialize process registration

  if (_params->trigger_mode != Params::FILE_LIST)
    PMU_auto_init(_progName, _params->instance,
		  PROCMAP_REGISTER_INTERVAL);

  return true;
}


/*********************************************************************
 * run() - run the program.
 */

void Chill2netCDFSweep::run()
{
  static const string method_name = "Chill2netCDFSweep::run()";
  
  while (!_dataTrigger->endOfData())
  {
    TriggerInfo trigger_info;
    
    if (_dataTrigger->next(trigger_info) != 0)
    {
      cerr << "ERROR: " << method_name << endl;
      cerr << "Error getting next trigger information" << endl;
      cerr << "Trying again...." << endl;
      
      continue;
    }
    
    _processFile(trigger_info.getFilePath());
    ++_sweepNumber;
    
  } /* endwhile - !_dataTrigger->endOfData() */
}


/**********************************************************************
 *              Private Member Functions                              *
 **********************************************************************/

/*********************************************************************
 * _allocateDataArrays() - Allocate space for the data arrays.
 */

void Chill2netCDFSweep::_allocateDataArrays(const int num_gates,
					    const int num_beams)
{
  // Make sure we aren't reducing the number of gates or beams.  Otherwise,
  // we'll have a segmentation fault inside the realloc method.

  int num_gates_needed = num_gates;
  int num_beams_needed = num_beams;
  
  if (num_gates_needed < _numGatesAlloc)
    num_gates_needed = _numGatesAlloc;
  if (num_beams_needed < _numBeamsAlloc)
    num_beams_needed = _numBeamsAlloc;
  
  // Do the reallocations, if needed.

  if (_numBeamsAlloc < num_beams_needed)
  {
    _reallocInfoArray(_azimuth, num_beams_needed);
    _reallocInfoArray(_elevation, num_beams_needed);
    _reallocInfoArray(_timeOffset, num_beams_needed);
  }
  
  if (_numGatesAlloc < num_gates_needed ||
      _numBeamsAlloc < num_beams_needed)
  {
    _reallocDataArray(_hAvgI, num_gates_needed, num_beams_needed);
    _reallocDataArray(_hAvgQ, num_gates_needed, num_beams_needed);
    _reallocDataArray(_hDbz, num_gates_needed, num_beams_needed);
    _reallocDataArray(_hVel, num_gates_needed, num_beams_needed);
    _reallocDataArray(_hNcp, num_gates_needed, num_beams_needed);
    _reallocDataArray(_hSw, num_gates_needed, num_beams_needed);
    _reallocDataArray(_hNiq, num_gates_needed, num_beams_needed);
    _reallocDataArray(_hAiq, num_gates_needed, num_beams_needed);
    _reallocDataArray(_hDm, num_gates_needed, num_beams_needed);
    
    _reallocDataArray(_vAvgI, num_gates_needed, num_beams_needed);
    _reallocDataArray(_vAvgQ, num_gates_needed, num_beams_needed);
    _reallocDataArray(_vDbz, num_gates_needed, num_beams_needed);
    _reallocDataArray(_vVel, num_gates_needed, num_beams_needed);
    _reallocDataArray(_vNcp, num_gates_needed, num_beams_needed);
    _reallocDataArray(_vSw, num_gates_needed, num_beams_needed);
    _reallocDataArray(_vNiq, num_gates_needed, num_beams_needed);
    _reallocDataArray(_vAiq, num_gates_needed, num_beams_needed);
    _reallocDataArray(_vDm, num_gates_needed, num_beams_needed);
  }
  
  _numGatesAlloc = num_gates_needed;
  _numBeamsAlloc = num_beams_needed;
}


/*********************************************************************
 * _initializeDataArrays() - Initialize all of the gates in the data
 *                           arrays to missing data values.  This is
 *                           done at the beginning of processing each
 *                           file to clear out the old data.
 */

void Chill2netCDFSweep::_initializeDataArrays()
{
  _initInfoArray(_azimuth);
  _initInfoArray(_elevation);
  _initInfoArray(_timeOffset);

  _initDataArray(_hAvgI);
  _initDataArray(_hAvgQ);
  _initDataArray(_hDbz);
  _initDataArray(_hVel);
  _initDataArray(_hNcp);
  _initDataArray(_hSw);
  _initDataArray(_hNiq);
  _initDataArray(_hAiq);
  _initDataArray(_hDm);
  
  _initDataArray(_vAvgI);
  _initDataArray(_vAvgQ);
  _initDataArray(_vDbz);
  _initDataArray(_vVel);
  _initDataArray(_vNcp);
  _initDataArray(_vSw);
  _initDataArray(_vNiq);
  _initDataArray(_vAiq);
  _initDataArray(_vDm);
}


/*********************************************************************
 * _processBeam() - Process the next beam in the given input file.
 *
 * Returns true on success, false on failure.
 */

bool Chill2netCDFSweep::_processBeam(FILE *input_file,
				     const string &input_file_path,
				     const bool first_beam)
{
  static const string method_name = "Chill2netCDFSweep::_processFile()";
}


/*********************************************************************
 * _processFile() - Process the given input file
 */

bool Chill2netCDFSweep::_processFile(const string &input_file_path)
{
  static const string method_name = "Chill2netCDFSweep::_processFile()";
  
  if (_params->debug)
    cerr << "*** Processing file: " << input_file_path << endl;

  // Open the input file

/*
  FILE *input_file;
  
  if ((input_file = fopen(input_file_path.c_str(), "r")) == 0)
  {
    cerr << "ERROR: " << method_name << endl;
    cerr << "Error opening input file: " << input_file_path << endl;
    
    return false;
  }
*/
  
// cinrad start
   int ret=scanIQFile(input_file_path.c_str(),&tshdr,swplist);
  // Process the beams in the file

   TSSweepHeader *swphdr=swplist.front();
  
 // write netcdf file 
   char ncfpath[256];
   sprintf(ncfpath,"%s.nc",input_file_path.c_str());
   Nc3File ncFile(ncfpath, Nc3File::Replace);
   if(!ncFile.is_valid())
   {
	   printf("invlaid\n");
   }
   int const  bn=400;//swphdr->binnum;
   int  const swpn=swplist.size();
   Nc3Dim *gateDim = ncFile.add_dim("Gates", bn);
   Nc3Dim *timeDim = ncFile.add_dim("Time", swpn);
   const Nc3Dim * dataDims2[]={gateDim,timeDim};
   Nc3Var *iVar = ncFile.add_var("I", nc3Float, 2,dataDims2);
   iVar->add_att("units", "I");
   Nc3Var *qVar = ncFile.add_var("Q", nc3Float, 2,dataDims2);
   qVar->add_att("units", "Q");
   Nc3Var *timeVar = ncFile.add_var("Time", nc3Int, timeDim);
   Nc3Var *azVar = ncFile.add_var("Azimuth", nc3Float, timeDim);
   Nc3Var *elVar = ncFile.add_var("Elevation", nc3Float, timeDim);
   Nc3Var *prtVar = ncFile.add_var("Prt", nc3Int, timeDim);
   Nc3Var *unixtimeVar = ncFile.add_var("UnixTime", nc3Int, timeDim);
   Nc3Var *nanotimeVar = ncFile.add_var("NanoSeconds", nc3Int, timeDim);
   long const dimSize=swpn*bn;
   vector<float >fi(dimSize),fq(dimSize),az(swpn),el(swpn);	
//   float  fi[dimSize],fq[dimSize];
   vector<int > swptime(swpn),nanotime(swpn),prt(swpn);
   SwpHdrList::iterator it;
   int swpidx=0;
   for(it=swplist.begin();it!=swplist.end()&&swpidx<swpn;it++,swpidx++)
   {
	   int offset=swpidx*bn;
	   for( int b=0;b<bn;b++)
	   {
		   fi[offset+b]=(*it)->iq[b][0];
		   fq[offset+b]=(*it)->iq[b][1];
	   }
	   swptime[swpidx]=(*it)->time_sec;
	   nanotime[swpidx]=(*it)->time_usec;
	  az[swpidx]=TSDecodeAngle((*it)->az);
	  el[swpidx]=TSDecodeAngle((*it)->el);
	  prt[swpidx]=1e6/(*it)->prf;

   }
   int x;
    x=timeVar->put(&swptime[0],swpn);
   //x=iVar->put(&fi[0],bn,swpn,1,1,1);
    x=qVar->put(&fq[0],bn,swpn);
    x=iVar->put(&fi[0],bn,swpn);
    azVar->put(&az[0],swpn);
    elVar->put(&el[0],swpn);
    prtVar->put(&prt[0],swpn);
    unixtimeVar->put(&swptime[0],swpn);
    nanotimeVar->put(&nanotime[0],swpn);

 //  ncFile.close();

  return true;
}
