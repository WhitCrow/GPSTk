//============================================================================
//
//  This file is part of GPSTk, the GPS Toolkit.
//
//  The GPSTk is free software; you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published
//  by the Free Software Foundation; either version 3.0 of the License, or
//  any later version.
//
//  The GPSTk is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with GPSTk; if not, write to the Free Software Foundation,
//  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
//  
//  Copyright 2004, The University of Texas at Austin
//
//============================================================================

//============================================================================
//
//This software developed by Applied Research Laboratories at the University of
//Texas at Austin, under contract to an agency or agencies within the U.S. 
//Department of Defense. The U.S. Government retains all rights to use,
//duplicate, distribute, disclose, or release this software. 
//
//Pursuant to DoD Directive 523024 
//
// DISTRIBUTION STATEMENT A: This software has been approved for public 
//                           release, distribution is unlimited.
//
//=============================================================================
/**
 * @file OrbAlmStore.cpp
 */

#include <iostream>
#include <fstream>
#include <iomanip>

#include "StringUtils.hpp"
#include "OrbAlmStore.hpp"
#include "MathBase.hpp"
#include "CivilTime.hpp"
#include "TimeString.hpp"

using namespace std;
using gpstk::StringUtils::asString;

namespace gpstk
{

//--------------------------------------------------------------------------

   Xvt OrbAlmStore::getXvt(const SatID& subjID, const CommonTime& t) const
      throw( InvalidRequest )
   {
      try
      {
         // test for GPS satellite system in sat?
         const OrbAlm* alm = findOrbAlm(subjID,t);

         // If the orbital elements are unhealthy, refuse to 
         // calculate an SV position and throw.
         if (!alm->healthy)
         {
            InvalidRequest exc( std::string("SV is transmitting unhealhty navigation ")
                + std::string("message at time of interest.") );
            GPSTK_THROW( exc );
         }
         Xvt sv = alm->svXvt(t);
         return sv;
      }
      catch(InvalidRequest& ir)
      {
         GPSTK_RETHROW(ir);
      }
   }

//------------------------------------------------------------------------------
      // This method is basically unimplemented at this level.   It may
      // be overriden by a descendent that needs to limit access to a 
      // certain system or systems. 
   bool OrbAlmStore::validSatSystem(const SatID& subjID) const 
   {
      return true;
   }

//--------------------------------------------------------------------------

   bool OrbAlmStore::isHealthy(const SatID& subjID, const CommonTime& t) const
      throw( InvalidRequest )
   {
      bool retVal = false;
      if (!validSatSystem(subjID))
      {
         stringstream ss;
         ss << subjID.convertSatelliteSystemToString(subjID.system) 
            << " is not a valid Satellite system for this OrbAlmStore.";
         InvalidRequest ir(ss.str());
         GPSTK_THROW(ir);
      }
      try
      {
         // test for GPS satellite system in sat?
         const OrbAlm* alm = findOrbAlm(subjID, t);
         
         retVal = alm->isHealthy();
      }
      catch(InvalidRequest& ir)
      {
         GPSTK_RETHROW(ir);
      }
      return retVal;
   } // end of OrbAlmStore::getHealth()

//--------------------------------------------------------------------------

   void OrbAlmStore::dump(std::ostream& s, short detail) const
      throw()
   {
      dumpSubjAlm(s,detail);
      dumpXmitAlm(s,detail);
   } // end OrbAlmStore::dump

//------------------------------------------------------------------------------------

//
//  Dtaill 0 = Only cout of objects per satellite 
//  Detail 1 = terse mode (one line)
//  Datial 2 = full dump of each object. 
void OrbAlmStore::dumpSubjAlm( std::ostream& s, short detail ) const
         throw()
{
      SubjectAlmMap::const_iterator it;
      static const string fmt("%04Y/%02m/%02d %02H:%02M:%02S %P");

      s << endl;
      s << "Dump of OrbAlmStore by satellite that is the SUBJECT of each almanac:\n";
      if (detail==0)
      {
         s << " Span is " << (initialTime == CommonTime::END_OF_TIME
                                      ? "End_time" : printTime(initialTime,fmt))
           << " to " << (finalTime == CommonTime::BEGINNING_OF_TIME
                                      ? "Begin_time" : printTime(finalTime,fmt))
           << " with " << size() << " entries."
           << std::endl;
      } // end if-block
      else if (detail==1)
      {
         for (it = subjectAlmMap.begin(); it != subjectAlmMap.end(); it++)
         {
            const OrbAlmMap& em = it->second;
            s << "  Almanac list for satellite " << it->first
              << " has " << em.size() << " entries." << std::endl;
            OrbAlmMap::const_iterator ei;

            for (ei = em.begin(); ei != em.end(); ei++) 
            {
               const OrbAlm* oe = ei->second;
               s << "PRN " << setw(2) << it->first
                 << " TOE " << printTime(oe->ctToe,fmt)
                 << " KEY " << printTime(ei->first,fmt);
               s << " begVal: " << printTime(oe->beginValid,fmt);
                
               s << std::endl;
            } //end inner for-loop */

         } // end outer for-loop
   
         //s << "  End of Orbit/Clock data." << std::endl << std::endl;

      } //end else-block

         // In this case the output is
         // key, beginValid,  Toe,   endValid
      else if (detail==2)
      {
         for (it = subjectAlmMap.begin(); it != subjectAlmMap.end(); it++)
         {
            const OrbAlmMap& em = it->second;
            s << endl;
            s << "  Almanac list for satellite " << it->first
              << " has " << em.size() << " entries." << std::endl;
            OrbAlmMap::const_iterator ei;

               // Get header from system-specific descendent of this class.
            s << getTerseHeader() << endl;
            //s << "          Epoch Time             " << endl;
            //s << " PRN  mm/dd/yyyy DOY hh:mm:ss   Health" << endl; 
            for (ei = em.begin(); ei != em.end(); ei++) 
            {
               const OrbAlm* oe = ei->second;
               oe->dumpTerse(s);
               s << std::endl;
            } //end inner for-loop */
         } // end outer for-loop
      }    // end of else-block
      else
      {
         for (it = subjectAlmMap.begin(); it != subjectAlmMap.end(); it++)
         {
            const OrbAlmMap& em = it->second;
            s << "  Almanac list for satellite " << it->first
              << " has " << em.size() << " entries." << std::endl;
            OrbAlmMap::const_iterator ei;

            for (ei = em.begin(); ei != em.end(); ei++) 
            {
               const OrbAlm* oe = ei->second;
               oe->dump(s);
            }
         }
      }
   } // end OrbAlmStore::dumpSubjAlm()

//
//  Dtaill 0 = Only cout of objects per satellite 
//  Detail 1 = terse mode (one line)
//  Detail 2 = full dump mode 
void OrbAlmStore::dumpXmitAlm( std::ostream& s, short detail ) const
         throw()
{
   XmitAlmMap::const_iterator   it;
   UniqueAlmMap::const_iterator it2;
   OrbAlmMap::const_iterator   it3; 
   static const string fmt("%04Y/%02m/%02d %02H:%02M:%02S %P");

  s << "Dump of OrbAlmStore by transmitting satellite:\n";
  if (detail==0)
  {
     s << " Span is " << (initialTime == CommonTime::END_OF_TIME
                                      ? "End_time" : printTime(initialTime,fmt))
       << " to " << (finalTime == CommonTime::BEGINNING_OF_TIME
                                      ? "Begin_time" : printTime(finalTime,fmt))
       << " with " << size(2) << " entries."
       << std::endl;
  } // end if-block
  else if (detail>=1)
  {
     for (it = xmitAlmMap.begin(); it != xmitAlmMap.end(); it++)
     {
        const SatID xmitID = it->first;
        s << endl;
        s << " List of almanacs received from " << xmitID << endl;; 
        const UniqueAlmMap& uam = it->second;
        multimap<CommonTime,OrbAlm*> tempMap; 

           // Need to construct a multimap of all the alamancs received, 
           // regardless of the satellite to which they are applicable.
        for (it2=uam.begin();it2!=uam.end();it2++)
        {
           const OrbAlmMap& oem = it2->second;
           for (it3=oem.begin();it3!=oem.end();it3++)
           {
              const CommonTime ct = it3->first;
              const OrbAlm* oeb = it3->second; 
              OrbAlm* px = oeb->clone();
              multimap<CommonTime,OrbAlm*>::value_type p(ct,px);
              tempMap.insert(p);
           }
        }
        
           // Get header from system-specific descendent of this class.
        if (detail==1) s << getTerseHeader() << endl;

           // The multimap was a convenient sorting method, but the 
           // order of the elements is unspecified.  Therefore, 
           // we are going to build a temporary map that allows
           // the subset we just selected to be output in transmit-time 
           // order.
           // FIRST - create the temporary ordered map. 
        OrbAlmMap tempMap2;
        multimap<CommonTime,OrbAlm*>::const_iterator cit;
        for (cit=tempMap.begin();cit!=tempMap.end();cit++)
        {
           const OrbAlm* oeb = cit->second; 
           CommonTime ct = oeb->beginValid; 
           OrbAlm* pxT = oeb->clone();
           multimap<CommonTime,OrbAlm*>::value_type pT(ct,pxT);
           tempMap2.insert(pT);
        }
           // SECOND - iterate over the tempoarary map. 
        OrbAlmMap::const_iterator citT;
        for (citT=tempMap2.begin();citT!=tempMap2.end();citT++)
        {
           const OrbAlm* oeb = citT->second; 
           if (detail==2) oeb->dump(s);
            else oeb->dumpTerse(s); 
           s << std::endl;          
        }
           // Then clear the temp maps
           // We cloned all these objects and need to free up the
           // memory. 
        OrbAlmMap::iterator zit;
        for (zit=tempMap2.begin();zit!=tempMap2.end();zit++)
        {
           OrbAlm* oeb = zit->second;
           delete oeb; 
        }
        tempMap2.clear();
        /*
        multimap<CommonTime,OrbAlm*>::iterator zit2;
        for (zit2=tempMap.begin();zit2!=tempMap.end();zit2)
        {
           OrbAlm* oeb = zit2->second;
           delete oeb; 
        }
        */
        tempMap.clear();

     }
  }
}

//------------------------------------------------------------------------------------
// Keeps only one OrbAlm for a given satellite and epoch time.
// It should keep the one with the earliest transmit time.
//
// There are two maps to be updated and the satellite ids associated
// with the OrbAlm object are not always the satellite IDs to be 
// uas as the key. 
//
// It is assumed that the provided OrbAlm already contains the 
// SatID of the subject satellite. 
//------------------------------------------------------------------------------------ 
   bool OrbAlmStore::addOrbAlm( const OrbAlm* alm, 
                                  const SatID& xmitID, 
                                  const SatID& subjID,
                                  const bool isXmitHealthy )
      throw(InvalidParameter,Exception)
   {
      bool test1 = false;
      bool test2 = false; 
      try
      {
            // First work on the subject almanac map.
         if (isXmitHealthy)
         {
            OrbAlmMap& oem = subjectAlmMap[subjID];
            test1 = addOrbAlmToOrbAlmMap(alm,oem);
         } 

            // Then work on the xmitAlmMap
         UniqueAlmMap& uam = xmitAlmMap[xmitID];
         OrbAlmMap& oemX = uam[subjID];
         test2 = addOrbAlmToOrbAlmMap(alm,oemX); 
      }
      catch(Exception& e)
      {
         GPSTK_RETHROW(e)
      }
      if (test1 || test2) return true;
      return false; 
   }

      // This is a protected method.  In this case, the appropriate
      // OrbAlmMap has already been selected by satllite.  Now the
      // question is where to add this element into that map.  
   bool OrbAlmStore::addOrbAlmToOrbAlmMap( const OrbAlm* alm, 
                                                    OrbAlmMap& oem)
         throw(InvalidParameter,Exception)
   {
   try
   {
      string ts = "%02m/%02d/%02y %02H:%02M:%02S";

         // Get the epoch time of the object
      const CommonTime& et = alm->ctToe;

/*
         // Search for epoch time in current keys.
         // There may be one or more candidates.
      OrbAlmMap::iterator it = oem.find(et);

         // If almanac for this epoch time doesn't already exist, 
         // we can simply load it. 
      if (it==oem.end())
      {
         oem[et] = alm->clone();
         return true;
      }
*/
         // Find the set of items with this epoch time already in table.
         // Several things to be checked for each candidate.
         //   1. Do the data contents match?  If so, we want to retain
         //      the earlier of the two.
         //   2. If not, proceed to the next candidate.
      pair<OrbAlmMap::iterator,OrbAlmMap::iterator> p = oem.equal_range(et);
      for (OrbAlmMap::iterator it=p.first;it!=p.second;it++)
      {
         const OrbAlm* oe = it->second;
         if (alm->isSameData(oe))
         {
            if (oe->beginValid <= alm->beginValid) return false;
            oem.erase(it);
            oem.insert(OrbAlmMap::value_type(et,alm->clone()));
            updateInitialFinal(alm);
            return true;
         }
      }

         // If the new almanac does not match any existing almanac, 
         // it will be added to the map.
      oem.insert(OrbAlmMap::value_type(et,alm->clone())); 

/*
      const OrbAlm* oe = it->second;
      if (!alm->isSameData(oe))
      {
           stringstream os;
           os << "Found matching epoch times, but different data. Satellite ";
           os << alm->satID.id;
           os << ", epoch time ";
           os <<  printTime(alm->ctToe,ts) << endl;
           os << "Currently in the Store:" << endl;
           oe->dump(os);
           os << endl; 
           os << "Object to be added:" << endl;
           alm->dump(os);
           os << endl;
           InvalidParameter exc( os.str() );
           GPSTK_THROW(exc); 
      }

         // The "new" item has a later transmit time the item already in the map.
         // Thereore, discard the new item and keep the one in the map.  
      if (oe->beginValid <= alm->beginValid) return false;

         // We need to erase the current item and insert the new item.
      oem.erase(it);
      oem[et] = alm->clone();
      updateInitialFinal(alm);
      return true;
*/
   }
   catch(Exception& e)
   {
      GPSTK_RETHROW(e)
   }
   return true;
   }
    
//-----------------------------------------------------------------------------

   void OrbAlmStore::edit(const CommonTime& tmin, const CommonTime& tmax)
      throw()
   {
      for(SubjectAlmMap::iterator i = subjectAlmMap.begin(); i != subjectAlmMap.end(); i++)
      {
         OrbAlmMap& eMap = i->second;

         OrbAlmMap::iterator lower = eMap.lower_bound(tmin);
         if (lower != eMap.begin())
         { 
            for (OrbAlmMap::iterator emi = eMap.begin(); emi != lower; emi++)
               delete emi->second;        
            eMap.erase(eMap.begin(), lower);
         } 

         OrbAlmMap::iterator upper = eMap.upper_bound(tmax);
         if (upper != eMap.end())
         {
            for (OrbAlmMap::iterator emi = upper; emi != eMap.end(); emi++)
               delete emi->second; 
            eMap.erase(upper, eMap.end());          
         }
      }

      for(XmitAlmMap::iterator i = xmitAlmMap.begin(); i != xmitAlmMap.end(); i++)
      {
         UniqueAlmMap& uam = i->second;
         UniqueAlmMap::iterator it2;
         for (it2=uam.begin();it2!=uam.end();it2++)
         {
            OrbAlmMap& eMap = it2->second;

            OrbAlmMap::iterator lower = eMap.lower_bound(tmin);
            if (lower != eMap.begin())
            { 
               for (OrbAlmMap::iterator emi = eMap.begin(); emi != lower; emi++)
                  delete emi->second;        
               eMap.erase(eMap.begin(), lower);
            } 

            OrbAlmMap::iterator upper = eMap.upper_bound(tmax);
            if (upper != eMap.end())
            {
               for (OrbAlmMap::iterator emi = upper; emi != eMap.end(); emi++)
                  delete emi->second; 
               eMap.erase(upper, eMap.end());          
            }
         }
      }

      initialTime = tmin;
      finalTime   = tmax;
   }

  void OrbAlmStore::clear()
         throw()
  {
        // First clear the subject almanac map
     SubjectAlmMap::iterator it;
     for (it=subjectAlmMap.begin();it!=subjectAlmMap.end();it++)
     {
        OrbAlmMap& oem = it->second;
        OrbAlmMap::iterator it2;
        for (it2=oem.begin();it2!=oem.end();it2++)
        {
           OrbAlm* oeb = it2->second;
           delete oeb;
        }
        oem.clear();
     }
     subjectAlmMap.clear();

        // Then clear the smit almanac map
     XmitAlmMap::iterator xit;
     for (xit=xmitAlmMap.begin();xit!=xmitAlmMap.end();xit++)
     {
        UniqueAlmMap& uam = xit->second;
        UniqueAlmMap::iterator xit2;
        for (xit2=uam.begin();xit2!=uam.end();xit2++)
        {
           OrbAlmMap& oem = xit2->second;
           OrbAlmMap::iterator xit3;
           for (xit3=oem.begin();xit3!=oem.end();xit3++)
           {
              OrbAlm* oeb = xit3->second;
              delete oeb;
           }
           oem.clear();
        }
        uam.clear();
     }
     xmitAlmMap.clear();
  }


//-----------------------------------------------------------------------------

   unsigned OrbAlmStore::size(unsigned short choice) const
         throw()
   {
      unsigned counter = 0;
      if (choice==0 || choice==1)
      {
         for(SubjectAlmMap::const_iterator i = subjectAlmMap.begin(); i != subjectAlmMap.end(); i++)
            counter += i->second.size();
      }

      if (choice==0 || choice==2)
      {
         for(XmitAlmMap::const_iterator i2 = xmitAlmMap.begin(); i2!=xmitAlmMap.end(); i2++)
         {
             UniqueAlmMap::const_iterator i3;
             const UniqueAlmMap& uam = i2->second;
             for (i3=uam.begin();i3!=uam.end();i3++)
             {
                counter += i3->second.size(); 
             }
         }
      }
      return counter;      
   }
   unsigned OrbAlmStore::sizeSubjAlm(const SatID& subjID) const
         throw()
   {
      unsigned counter = 0;
      SubjectAlmMap::const_iterator i = subjectAlmMap.find(subjID);
      if (i!=subjectAlmMap.end())
      {
         const OrbAlmMap& oem = i->second;
         counter = oem.size();
      }
      return counter;
   }

   unsigned OrbAlmStore::sizeXmitAlm(const SatID& xmitID) const
         throw()
   {
      unsigned counter = 0; 
      XmitAlmMap::const_iterator i2 = xmitAlmMap.find(xmitID);
      if (i2!=xmitAlmMap.end())
      {
          UniqueAlmMap::const_iterator i3;
          const UniqueAlmMap& uam = i2->second;
          for (i3=uam.begin();i3!=uam.end();i3++)
          {
             counter += i3->second.size(); 
          }
      }
      return counter;
   } 

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Goal is to find the set of orbital elements that would have been
// used by a receiver in real-time.   That is to say, the most recently
// broadcast elements (assuming the receiver has visibility to the SV
// in question at the time of interest).
// Since this case addresses almanac data, there is NO concern regarding
// period of effectivity.  The method simply looks for the most recently
// transmitted almanac.   Unfortunately, given that the KEY is the 
// epoch time, that search is not a simple find.   HOWEVER, we want to 
// keep the map in time-order by epoch time due to other concerns. 
//-----------------------------------------------------------------------------

   const OrbAlm*
   OrbAlmStore::findOrbAlm(const SatID& subjID, const CommonTime& t) const
      throw( InvalidRequest )
   {
          // Check to see that there exists a map of orbital elements
     // relevant to this SV.
      SubjectAlmMap::const_iterator prn_i = subjectAlmMap.find(subjID);
      if (prn_i == subjectAlmMap.end())
      {
         InvalidRequest e("No orbital elements for satellite " + asString(subjID));
         GPSTK_THROW(e);
      }

         // Define reference to the relevant map of orbital elements
      const OrbAlmMap& em = prn_i->second;
      OrbAlmMap::const_iterator cit; 

         // If there is only one element, that's the one we are going to return.
      if (em.size()==1)
      {
         cit = em.begin();
         return cit->second; 
      }

         // For the moment, I'm implementing a dirt-stupid linear search
         // from the beginning of the map.  If we ever want to process
         // weeks-and-weeks of almanac data at once, we will want to 
         // reconsider this. 
      OrbAlmMap::const_iterator previt; 
      bool first = true; 
      for (cit=em.begin();cit!=em.end();cit++)
      {
         OrbAlm* testp = cit->second;
         if (first)
         {
               // If the very first item in the map has a transmit time
               // later than the time of interest, then the best we can 
               // do is return that item.
            if (testp->beginValid >= t) return cit->second; 

               // Otherwise, store this pointer as a POSSIBLE candidate, 
               // and move on the next item. 
            previt = cit;
            first = false;
         }
         else
         {
            if (testp->beginValid >= t) return previt->second;
            previt = cit; 
         }
      }
         // We reached the end of the map without finding a transmit
         // time beyond the time of interest.  Return the last item
         // in the map. 
      return previt->second; 
   } 
 
      // Question: Do we really need/want this method?  If so, there's
      // somewhat of a conflict for "best design" between the need for 
      // a decent lookup algorithm and the need for an appropriate 
      // orderly dump. 
   const OrbAlm* OrbAlmStore::findOrbAlm( const SatID& xmitID, 
                                                 const SatID& subjID, 
                                                 const CommonTime& t )
         const throw( InvalidRequest )
   {
      OrbAlm* oeb = 0;
      return oeb; 
   }
         
//-----------------------------------------------------------------------------

   const OrbAlmStore::OrbAlmMap&
   OrbAlmStore::getOrbAlmMap( const SatID& subjID ) const
      throw( InvalidRequest )
   {
      if (!validSatSystem(subjID))
      {
         InvalidRequest e("Incorrect satellite system requested.");
         GPSTK_THROW(e);
      }

      SubjectAlmMap::const_iterator prn_i = subjectAlmMap.find(subjID);
      if (prn_i == subjectAlmMap.end())
      {
         InvalidRequest e("No OrbAlm for satellite " + asString(subjID));
         GPSTK_THROW(e);
      }
      return(prn_i->second);
   }
   
} // namespace
