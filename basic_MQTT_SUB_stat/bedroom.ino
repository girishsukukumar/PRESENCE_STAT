

typedef struct BedRoomMetrics
{

   int BedRoomMovementsCounter  ;
   	 	 
	 float S_no_of_bedRoomMovements = 0.0;
	 float K_no_of_bedRoomMovements = 0.0;
	 float H_no_of_bedRoomMovements = 0.0;

} BedRoomMetrics_t ;

struct BedRoomMetrics BedRoomData ;


bool GetOccupancyStateOfRoom(const char* roomName) ;




bool CU_SUM_Update_BedRoomMovements()
{
    /*
    x -> current data
    median -> 	TrainedData->median_no_of_bedRoomMovements
		S_upper-> BathRoomData->S_no_of_bedRoomMovements
		k ->  BathRoomData->K_no_of_bedRoomMovements = 0.5 * TrainedData->MAD_no_of_bedRoomMovements
		h ->  BathRoomData->H_no_of_bedRoomMovements = 5 * TrainedData->MAD_no_of_bedRoomMovements
		
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
	
	delta = BedRoomData.BedRoomMovementsCounter - TrainedData.median_no_of_bedRoomMovements ;
	
	ans =   BedRoomData.S_no_of_bedRoomMovements + delta - (int) BedRoomData.K_no_of_bedRoomMovements ;
	
	if (ans > 0)
	{
		  BedRoomData.S_no_of_bedRoomMovements = ans ;
	}
	else
	{
		  BedRoomData.S_no_of_bedRoomMovements = 0 ;
  }
	
	if (BedRoomData.S_no_of_bedRoomMovements >  BedRoomData.H_no_of_bedRoomMovements)
  {		
      trend_anomaly = true ; 
  }		

  DEBUG_PRINTF("CUSUM-BedRoom, S = %f, k =%f , h = %f\n", BedRoomData.S_no_of_bedRoomMovements,
                                                          BedRoomData.K_no_of_bedRoomMovements,
                                                          BedRoomData.H_no_of_bedRoomMovements) ;
  return trend_anomaly ;
}

void InitBedRoomData()
{
    int MAD = Get_MAD_no_of_bedRoomMovements_from_training_data();
    BedRoomData.K_no_of_bedRoomMovements = 0.5 * MAD;
    BedRoomData.H_no_of_bedRoomMovements = 5 * MAD ;
    BedRoomData.BedRoomMovementsCounter = 0 ; 
}

void incrementBedRoomMovementCounter()
{
    BedRoomData.BedRoomMovementsCounter++ ;
}

void RunAnomalyCheckForBedRoom()
{
   // If too many movements are detected 
    bool trend_flag ;
    trend_flag = CU_SUM_Update_BedRoomMovements() ;
   if (trend_flag == true)
   {
        publishAlert("BedRoom", "Unusal Movements detected in BedRoom during night");
        /* 
           Reset the movement counter so that the alert don't get triggered  
           Increment the limit to avoid period alert , if needed 
        */

        BedRoomData.BedRoomMovementsCounter= 0 ; 
   }
}

void CheckProlongedUseOfBedRoom()
{
      time_t diff ;
      int idx = findRoomIndex("BedRoom");
      time_t now = getEpochTime() ;
      

      if (GetOccupancyStateOfRoom("BedRoom") == true)
      {
        /* Check the duration of occupancy */
         diff = now - GetOccupiedEpocTimeOfRoom("BedRoom");

         if (diff > TrainedData.BEDROOM_MAX_DURATION) 
         {
              publishAlert("Toilet-1", "Prolonged Use of BedRoom detected");
         }
         
      }
}