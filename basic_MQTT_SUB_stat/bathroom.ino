#define JSON_TRAINING_FILE_NAME "/TrainingData.json"



bool GetOccupancyStateOfRoom(const char* roomName) ;

typedef struct BathRoomMetrics
{
	int total_no_of_day_visits = 0 ;     /* 7 AM to 8PM  */
	int total_no_of_night_visits = 0 ;   /* 8 PM to 7 AM */
	int duration_of_day_visits[50];
	int duration_of_night_visits[50];
	 
	 /* following are variable for calculating the trend
	    using CU-SUM Cumiltive Sum Method
	  */
	  
	float S_no_of_day_visits = 0.0 ;
	float k_no_of_day_visits ;
	float h_no_of_day_visits ;
	 
	 	 
	float S_no_of_night_visits = 0.0;
	float k_no_of_night_visits ;
	float h_no_of_night_visits ;
	 
	 	
	bool trend_flag_day_vists = false ;
	bool trend_flag_night_vists = false ;
	

	bool spike_flag_for_no_day_vists = false  ; 
	bool spike_flag_for_no_night_vists = false ;
	bool spike_flag_for_duration_day_vists = false ;
	bool spike_flag_for_duration_night_vists = false;
	
	int alertCount =  0; 
	 
} bathRoomMetrics_t;

bathRoomMetrics_t BathRoomData ;


typedef struct TrainingData
{
	int mean_no_of_day_visits ;
	int mean_no_of_night_visits ; 

	int median_no_of_day_visits;
	int median_no_of_night_visits ;
	
	int MAD_no_of_day_visits;      /* (median absolute deviation) */
	int MAD_no_of_night_visits ;   /* (median absolute deviation) */
	
	int mean_duration_of_day_visits ;
	int mean_duration_of_night_visits ;
	
	int median_duration_of_day_visits ;
	int median_duration_of_night_visits ;

	int MAD_duration_of_day_visits ;   /* (median absolute deviation) */
	int MAD_duration_of_night_visits ; /* (median absolute deviation) */

  int median_no_of_bedRoomMovements ;
  int MAD_no_of_bedRoomMovements;


  
  unsigned long BEDROOM_MAX_DURATION ;
  unsigned long BEDROOM_MOVEMENT_SCORE_LIMIT ; 
  unsigned long MAX_LIVINGROOM_DURATION_NIGHT ;
  unsigned long MAX_KITCHEN_DURATION_NIGHT ; 
  unsigned long MAX_BATHROOM_DURATION ;
  unsigned long BATHROOM_ALERT_THRESHOLD ;
  unsigned long DAY_TIME_BATHROOM_VISIT_BASELINE ;
	
} trainingData_t;

trainingData_t  TrainedData ;

void IncrementBathRoomDayVisits()
{
  DEBUG_PRINTF("IncrementBathRoomDayVisits\n") ;
	BathRoomData.total_no_of_day_visits++; 
}
void IncrementBathRoomNightVisits()
{
  DEBUG_PRINTF("IncrementBathRoomNightVisits\n") ;
	BathRoomData.total_no_of_night_visits++ ;
}

void UpdateBathRoomDayVisitDuration(int duration)
{
	BathRoomData.duration_of_day_visits[BathRoomData.total_no_of_day_visits-1] = duration ;
}
void UpdateBathRoomNightVisitDuration(int duration)
{
	BathRoomData.duration_of_night_visits[BathRoomData.total_no_of_night_visits-1] = duration ;
}

/* *****************************************************************************
Function Name : LoadTrainingData
INPUT PARAMETERS :
    JSON FILE WITH TRAINED DATA VALUES
RETURN
   None
FUNCTION
   Read the JSON file and load the trained values/weights into data structure  struct TrainingData
Author : Girish Kumar
******************************************************************************/

void LoadTrainingData()
{
	  File jsonFile ;
     
    JsonDocument  doc;

    DEBUG_PRINTF("LoadBathRoomTrainingData starting\n") ;
    jsonFile = SD.open(JSON_TRAINING_FILE_NAME, FILE_READ);
  
    if (!jsonFile)
    {
        DEBUG_PRINTF("Unable to open %s\n",JSON_TRAINING_FILE_NAME);
       return ;
    }
    deserializeJson(doc, jsonFile);
	
	  const char* mean_no_of_day_visitsCharPtr   = doc["mean_no_of_day_visits"] ; // "xxxxxxxxxxxxxxxxxxxx"
    
    const char* mean_no_of_night_visitsCharPtr = doc["mean_no_of_night_visits"] ; // "xxxxxxxxxxxxxxxxxxxx"
       
    const char* median_no_of_day_visitsCharPtr = doc["median_no_of_day_visits"] ; // "xxxxxxxxxxxxxxxxxxxx"
  
    const char* median_no_of_night_visitsCharPtr = doc["median_no_of_night_visits"] ; // "xxxxxxxxxxxxxxxxxxxx"

    const char* MAD_no_of_day_visitsCharPtr    = doc["MAD_no_of_day_visits"] ; // "xxxxxxxxxxxxxxxxxxxx"
  
    const char* MAD_no_of_night_visitsCharPtr  = doc["MAD_no_of_night_visits"] ; // "xxxxxxxxxxxxxxx"  

    const char* mean_duration_of_day_visitsCharPtr = doc["mean_duration_of_day_visits "] ; // "xxxxxxxxxxxxxx"
  
    const char* mean_duration_of_night_visitsCharPtr = doc["mean_duration_of_night_visits"] ; // "xxxxxxxxxxxxxx"

    const char* median_duration_of_day_visitsCharPtr = doc["median_duration_of_day_visits"] ;
  
    const char* median_duration_of_night_visitsCharPtr = doc["median_duration_of_night_visits"] ;

	  const char* MAD_duration_of_day_visitsCharPtr = doc["MAD_duration_of_day_visits"] ;
	
    const char* MAD_duration_of_night_visitsCharPtr = doc["MAD_duration_of_night_visits"];

    /* Parameters for BedRoom */
    const char* median_no_of_bedRoomMovementsCharPtr =  doc["median_no_of_bedRoomMovements"] ;
  
    const char* MAD_no_of_bedRoomMovementsCharPtr =    doc["MAD_no_of_bedRoomMovements"];
   /* End Parameters for BedRoom */
    

    const char* BEDROOM_MAX_DURATIONCharPtr = doc["BEDROOM_MAX_DURATION"];
  
    const char* BEDROOM_MOVEMENT_SCORE_LIMITCharPtr = doc["BEDROOM_MOVEMENT_SCORE_LIMIT"];  
  
    const char* MAX_LIVINGROOM_DURATION_NIGHTCharPtr = doc["MAX_LIVINGROOM_DURATION_NIGHT"]; 
  
    const char* MAX_KITCHEN_DURATION_NIGHTCharPtr = doc["MAX_KITCHEN_DURATION_NIGHT"];
  
    const char* MAX_BATHROOM_DURATIONCharPtr = doc["MAX_BATHROOM_DURATION"];
  
    const char* BATHROOM_ALERT_THRESHOLDCharPtr =   doc["BATHROOM_ALERT_THRESHOLD"];
  
    const char* DAY_TIME_BATHROOM_VISIT_BASELINECharPtr = doc["DAY_TIME_BATHROOM_VISIT_BASELINE"]; 
	
	  TrainedData.mean_no_of_day_visits = String(mean_no_of_day_visitsCharPtr).toInt();
      
    TrainedData.mean_no_of_night_visits = String(mean_no_of_night_visitsCharPtr).toInt();
	
	  TrainedData.median_no_of_day_visits =  String(median_no_of_day_visitsCharPtr).toInt();
	  
    TrainedData.median_no_of_night_visits = String(median_no_of_night_visitsCharPtr).toInt();
	
	  TrainedData.MAD_no_of_day_visits =   String(MAD_no_of_day_visitsCharPtr).toInt();
	  
    TrainedData.MAD_no_of_night_visits = String(MAD_no_of_night_visitsCharPtr).toInt();
	
    /* Parameters for BedRoom */
    
	  TrainedData.median_no_of_bedRoomMovements =   String(median_no_of_bedRoomMovementsCharPtr).toInt() ;
	  
    TrainedData.MAD_no_of_bedRoomMovements = String(MAD_no_of_bedRoomMovementsCharPtr).toInt() ;

	  
    /*  Parameters for BedRoom */
 
	  TrainedData.median_duration_of_day_visits = String(median_duration_of_day_visitsCharPtr).toInt();
	  
    TrainedData.median_duration_of_night_visits = String(median_duration_of_night_visitsCharPtr).toInt();
	
	  TrainedData.MAD_duration_of_day_visits  = String(MAD_duration_of_day_visitsCharPtr).toInt();
	  
    TrainedData.MAD_duration_of_night_visits  = String(MAD_duration_of_night_visitsCharPtr).toInt();
	
	  BathRoomData.k_no_of_day_visits = 0.5 * TrainedData.MAD_no_of_day_visits ;
    
    BathRoomData.h_no_of_day_visits = 5 * TrainedData.MAD_no_of_day_visits ;
	
	  BathRoomData.k_no_of_night_visits = 0.5 * TrainedData.MAD_no_of_night_visits ;
    
    BathRoomData.h_no_of_night_visits = 5 * TrainedData.MAD_no_of_night_visits ;

    TrainedData.BEDROOM_MAX_DURATION = String(BEDROOM_MAX_DURATIONCharPtr).toInt();

    TrainedData.BEDROOM_MOVEMENT_SCORE_LIMIT  = String(BEDROOM_MOVEMENT_SCORE_LIMITCharPtr).toInt(); 
    
    TrainedData.MAX_LIVINGROOM_DURATION_NIGHT  = String(MAX_LIVINGROOM_DURATION_NIGHTCharPtr).toInt();
    
    TrainedData.MAX_KITCHEN_DURATION_NIGHT = String(MAX_KITCHEN_DURATION_NIGHTCharPtr).toInt();
    
    TrainedData.MAX_BATHROOM_DURATION  = String(MAX_BATHROOM_DURATIONCharPtr).toInt();
    
    TrainedData.BATHROOM_ALERT_THRESHOLD = String(BATHROOM_ALERT_THRESHOLDCharPtr).toInt();
    
    TrainedData.DAY_TIME_BATHROOM_VISIT_BASELINE  = String(DAY_TIME_BATHROOM_VISIT_BASELINECharPtr).toInt();

    jsonFile.close();
}


int Get_MAD_no_of_bedRoomMovements_from_training_data()
{
   return  TrainedData.MAD_no_of_bedRoomMovements ;
}

void printTrainedDataForBathRoom()
{
   	
	DEBUG_PRINTF("mean_no_of_day_visits = %d\n",TrainedData.mean_no_of_day_visits) ;
	DEBUG_PRINTF("mean_no_of_night_visits = %d\n",TrainedData.mean_no_of_night_visits) ;
	
	DEBUG_PRINTF("median_no_of_day_visits = %d\n",TrainedData.median_no_of_day_visits) ;
	DEBUG_PRINTF("median_no_of_night_visits = %d\n",TrainedData.median_no_of_night_visits) ;
	
	DEBUG_PRINTF("MAD_duration_of_day_visits = %d\n",TrainedData.MAD_duration_of_day_visits);
	DEBUG_PRINTF("MAD_no_of_night_visits = %d\n",TrainedData.MAD_no_of_night_visits)  ;
	
	DEBUG_PRINTF("mean_duration_of_day_visits = %d\n",TrainedData.mean_duration_of_day_visits) ;
	DEBUG_PRINTF("mean_duration_of_night_visits = %d\n",TrainedData.mean_duration_of_night_visits) ;
	
	DEBUG_PRINTF("median_duration_of_day_visits = %d\n",TrainedData.median_duration_of_day_visits) ;
	DEBUG_PRINTF("median_duration_of_night_visits = %d\n",TrainedData.median_duration_of_night_visits) ;
	
	DEBUG_PRINTF("MAD_duration_of_day_visits = %d\n",TrainedData.MAD_duration_of_day_visits)  ;
	DEBUG_PRINTF("MAD_duration_of_night_visits = %d\n",TrainedData.MAD_duration_of_night_visits);
	
  DEBUG_PRINTF("k_no_of_day_visits = %f\n",BathRoomData.k_no_of_day_visits);
  DEBUG_PRINTF("h_no_of_day_visits =%f\n" ,BathRoomData.h_no_of_day_visits);
	
	DEBUG_PRINTF("k_no_of_night_visits = %f\n",BathRoomData.k_no_of_night_visits);
  DEBUG_PRINTF("h_no_of_night_visits = %f\n", BathRoomData.h_no_of_night_visits);
}

/* ------------------------------------------------------------------------------
 ****************************** SPIKE DETECTION ALGO***************************
---------------------------------------------------------------------------------*/

/* *****************************************************************************
Function Name : modified_z
INPUT PARAMETERS :
    int x,  : data
    int median,  : Median
    float mad  : Mean Avg Deviation
RETURN
   Modified Value of Z
FUNCTION
   Calculate the modified value of Z based on input parameters.
Author : Girish Kumar
******************************************************************************/
float modified_z(int x, int median, float mad)
{ 
  float modifiedZ ;
  if (mad == 0.0)
	{
        return 0.0 ; 
	}
  if (x == median) 
	{
	   return 0.0 ;
  }

    modifiedZ  =  0.6745 * (x - median) / mad ;
	 return modifiedZ  ;
}

bool SpikeDetection()
{
	
   int idx ;

	  bool master_spike_flag = false ;
	
    float z_freq_no_day_vists  = (float) modified_z(BathRoomData.total_no_of_day_visits,
  	                                                TrainedData.median_no_of_day_visits, 
								                	                  TrainedData.MAD_no_of_day_visits) ;
	 
    float z_freq_no_night_vists  = (float) modified_z(BathRoomData.total_no_of_night_visits, 
	                                                    TrainedData.median_no_of_night_visits, 
										                                  TrainedData.MAD_no_of_night_visits) ;
			
    idx =  BathRoomData.total_no_of_day_visits -1 ;
	
	  float z_freq_duration_day_vists = (float) modified_z(BathRoomData.duration_of_day_visits[idx], 
	                                                       TrainedData.median_duration_of_day_visits, 
											                                   TrainedData.MAD_duration_of_day_visits) ;
    idx =  BathRoomData.total_no_of_night_visits -1 ;							
	
	  float z_freq_duration_night_vists = (float) modified_z(BathRoomData.duration_of_night_visits[idx],
                                                           TrainedData.median_duration_of_night_visits, 
											                                     TrainedData.MAD_duration_of_night_visits) ;

    
    DEBUG_PRINTF("z_no_of_day_vists =%f, z_no_of_night_vists=%f, z_dur_day_vist=%f, z_dur_night_visit %f\n",
                   z_freq_no_day_vists,
                   z_freq_no_night_vists,
                   z_freq_duration_day_vists,
                   z_freq_duration_night_vists) ;
	
	//Spike_flag = (abs(z_freq) > 3.5) or (z_night > 3.5) or (max(z_durs, default=0) > 3.5)
	 
	BathRoomData.spike_flag_for_no_day_vists = false; 
	BathRoomData.spike_flag_for_no_night_vists =false;
	BathRoomData.spike_flag_for_duration_day_vists =false;
	BathRoomData.spike_flag_for_duration_night_vists = false ;
	 
	if ((abs(z_freq_no_day_vists)> 3.5))
	{
        BathRoomData.spike_flag_for_no_day_vists = true ; 
	}
	
	if ((abs(z_freq_no_night_vists)> 3.5))
	{
	    BathRoomData.spike_flag_for_no_night_vists = true ;
	}
	
	if ((max((int)z_freq_duration_day_vists,0)> 3.5) == true)
	{
	    BathRoomData.spike_flag_for_duration_day_vists = true ;
	}
	
	if ((max((int)z_freq_duration_night_vists,0)> 3.5)==true)
	{
	    BathRoomData.spike_flag_for_duration_night_vists = true ;
	}
	
	if ((BathRoomData.spike_flag_for_no_day_vists) || 
	   (BathRoomData.spike_flag_for_no_night_vists) ||
	   (BathRoomData.spike_flag_for_duration_day_vists) || 
		 (BathRoomData.spike_flag_for_duration_night_vists))
	{
      master_spike_flag = true ;
  }

  return  master_spike_flag ;
}

/* ------------------------------------------------------------------------------
 ****************************** TREND DETECTION ALGO***************************
---------------------------------------------------------------------------------*/

bool CU_SUM_Update_no_of_day_visits()
{
    /*
        x -> current data
        median -> 	TrainedData->median_no_of_day_visits
		S_upper-> BathRoomData->S_no_of_day_visits
		k ->  BathRoomData->k_no_of_day_visits = 0.5 * TrainedData->MAD_no_of_day_visits
		h ->  BathRoomData->h_no_of_day_visits = 5 * TrainedData->MAD_no_of_day_visits
		
		Algorithm
		function cusum_update(x, median, S_upper, k, h)
		{
            S_upper = max(0, S_upper + (x - median) - k)
            trend_anomaly = S_upper > h
            return S_upper, trend_anomaly
		}
	*/
	
	int delta ;
	bool trend_anomaly = 0 ;
	int  ans ;
	
	delta = BathRoomData.total_no_of_day_visits - TrainedData.median_no_of_day_visits ;
	
	ans = BathRoomData.S_no_of_day_visits + delta - (int) BathRoomData.k_no_of_day_visits ;
	
	if (ans > 0)
	{
		BathRoomData.S_no_of_day_visits = ans ;
	}
	else
	{
		BathRoomData.S_no_of_day_visits = 0 ;
  }
	
	if (BathRoomData.S_no_of_day_visits >  BathRoomData.h_no_of_day_visits)
    {		
        trend_anomaly = true ; 
    }		

    DEBUG_PRINTF("CUSUM-day, S = %f, k =%f , h = %f\n",BathRoomData.S_no_of_day_visits,
                                                     BathRoomData.k_no_of_day_visits,
                                                     BathRoomData.h_no_of_day_visits ) ;
    return trend_anomaly ;
}


bool CU_SUM_Update_no_of_night_visits()
{
	int delta ;
	bool trend_anomaly = 0 ;
	int  ans ;
	
  /* 
      DEBUG_PRINTF("CU_SUM_Update_no_of_night_visits: no_of_night_visits = %d, media =%d \n",
                 BathRoomData.total_no_of_night_visits,
                TrainedData.median_no_of_night_visits) ;
   */

	delta = BathRoomData.total_no_of_night_visits - TrainedData.median_no_of_night_visits ;
	
   /*
     DEBUG_PRINTF("CU_SUM_Update_no_of_night_visits: S_no_of_night_visits = %f, k_night_visits =%f \n",
                 BathRoomData.S_no_of_night_visits,
                 BathRoomData.k_no_of_night_visits) ;
    */
	ans = BathRoomData.S_no_of_night_visits + delta - (int) BathRoomData.k_no_of_night_visits ;
	
	if (ans > 0)
	{
		BathRoomData.S_no_of_night_visits = ans ;
	}
	else
	{
		BathRoomData.S_no_of_night_visits = 0 ;
  }
	
	if (BathRoomData.S_no_of_night_visits >  BathRoomData.h_no_of_night_visits)
  {		
        trend_anomaly = true ; 
  }		

  DEBUG_PRINTF("CUSUM-night, S = %f, k =%f , h = %f\n",BathRoomData.S_no_of_night_visits,
                                                     BathRoomData.k_no_of_night_visits,
                                                     BathRoomData.h_no_of_night_visits) ;
  /*
   DEBUG_PRINTF("CU_SUM_Update_no_of_night_visits: S_no_of_night_visits = %f anomaly%d\n", BathRoomData.S_no_of_night_visits,trend_anomaly);
   */

   return trend_anomaly ;

}

bool TrendDetection()
{
	bool master_trend_flag = false ;
	/* ---------------------------------------------------------------
	Use a one-sided CU-SUM  (Cumulative-Sum ) on each metric to catch gradual upward drift 
	on the number of visits during day time and night time
	We are not looking at duration of visiting using this method a different method will 
	be used
	
	(e.g. nighttime visits creeping up):
	
	How does CU-SUM works
	
	1. When a data  is stable and running within target, positive and negative deviations 
	    cancel out, causing the CUSUM line to hover horizontally around
	
	2. If the data average shifts higher or lower, the deviations accumulate in one direction, 
	    causing the line to slope up or down to signal an out-of-control condition
		
	CU-SUM will be used only for detecting trends on two parameters
	
	1. total_no_of_day_visits;    
	2. total_no_of_night_visits;  
	
	----------------------------------------------------------------*/
	
	BathRoomData.trend_flag_day_vists = CU_SUM_Update_no_of_day_visits();
	BathRoomData.trend_flag_night_vists = CU_SUM_Update_no_of_night_visits();
	
	if ((BathRoomData.trend_flag_day_vists == true) || (BathRoomData.trend_flag_night_vists ==true))
	{
		master_trend_flag = true ;
	}
	else
	{
		master_trend_flag = false ;
	}
	return master_trend_flag ;
	
} 

void InitBathRoomUsageData()
{
  /*
       As the system is running constantly, the observation of bathroom usage need to reset daily
       So that it does not keet accumulating. The function implements the logic to reset
       bathroom data.
      
    */
    
    int idx ;

    BathRoomData.total_no_of_day_visits = 0 ;  
	  BathRoomData.total_no_of_night_visits = 0;  
    
    for (idx=0;idx<50;idx++)
    { 
	     BathRoomData.duration_of_day_visits[idx] = 0;
	     BathRoomData.duration_of_night_visits[idx] = 0;
    }
 

  return ;
}

void PrintBathRoomUsageData()
{
   int idx ;
   DEBUG_PRINTF("total_no_of_day_visits %d\n",BathRoomData.total_no_of_day_visits) ;
   DEBUG_PRINTF("total_no_of_night_visits  %d\n",BathRoomData.total_no_of_night_visits) ;
   DEBUG_PRINT("duration_of_day_visits  ") ;
     
   for (idx = 0 ; idx< 50; idx++)
   {
      DEBUG_PRINT(BathRoomData.duration_of_day_visits[idx]) ;
      DEBUG_PRINT(", ") ;    
   }
   DEBUG_PRINTF("\n") ;
   DEBUG_PRINT("duration_of_night_visits  ") ;
     
   for (idx = 0 ; idx< 50; idx++)
   {
      DEBUG_PRINT(BathRoomData.duration_of_night_visits[idx]) ; 
      DEBUG_PRINT(", ") ;      
   }
   DEBUG_PRINTF("\n") ;
}

void RunAnomalyCheckForBathroomUsage()
{
    
	
	/* 
	   #1. Get Training data from SD card
	   #2. Spike detection
	   #3. Trend detection (CUSUM state persisted across days)
       #4. Persistence filter on spike flags
	   #5. Final decision
	   
	 */
	 bool spike_detected, changeInTrendDetected ;
	
	 spike_detected = SpikeDetection() ;
	 
   changeInTrendDetected = TrendDetection() ;
	 
   DEBUG_PRINTF("RunAnomalyCheckForBathroomUsage \n") ;

	 if ((spike_detected == true) || (changeInTrendDetected == true))
	 {
	
  	 BathRoomData.alertCount++ ;
		 
		 if (BathRoomData.alertCount > TrainedData.BATHROOM_ALERT_THRESHOLD)
		 {
		    DEBUG_PRINTF("Alert: Bathroom readings not normal");
        if (changeInTrendDetected == true)
        {

           if (BathRoomData.trend_flag_day_vists == true)
           {
              publishAlert("Toilet-1", "Variation in trend during day Observed");
           }
           if (BathRoomData.trend_flag_night_vists == true)
           {
               publishAlert("Toilet-1", "Variation in trend during night Observed");
           }
           PrintBathRoomUsageData();
           InitBathRoomUsageData();
           BathRoomData.alertCount = 0 ; 
        }
        if (spike_detected == true)
        {
          if (BathRoomData.spike_flag_for_no_day_vists == true)
          {
               publishAlert("Toilet-1", "Spike in no of toilet visits during day");
          }  
	        if (BathRoomData.spike_flag_for_no_night_vists == true)
          {
              publishAlert("Toilet-1", "Spike in no of toilet visits  during night");
          }
	        if (BathRoomData.spike_flag_for_duration_day_vists == true)
          {
              publishAlert("Toilet-1", "Increase in duration of toilet usage during day");
          }
	        if (BathRoomData.spike_flag_for_duration_night_vists == true)
          {
             publishAlert("Toilet-1", "Increase in duration of toilet usage during night"); 
          }
        }
        
        PrintBathRoomUsageData();
        InitBathRoomUsageData(); 
			  BathRoomData.alertCount = 0 ; 
		 }
	 }	 	 
}


void CheckProlongedUseOfBathRoom()
{
      time_t diff ;

      time_t now = getEpochTime() ;
      
      if (GetOccupancyStateOfRoom("Toilet-1") == true)
      {
        /* Check the duration of occupancy */

         diff = now - GetOccupiedEpocTimeOfRoom("Toilet-1");

         if (diff > TrainedData.MAX_BATHROOM_DURATION) 
         {
              publishAlert("Toilet-1", "Prologed  Use of Bath room detected");
         }

      }
}
