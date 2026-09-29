

void RunAnomalyCheckForKitchen()
{
      time_t diff ;
      int idx = findRoomIndex("Kitchen");
      time_t now = getEpochTime() ;
      

      if (GetOccupancyStateOfRoom("Kitchen") == true)
      {
        /* Check the duration of occupancy */
         diff = now - GetOccupiedEpocTimeOfRoom("Kitchen");
         
         if (diff > TrainedData.MAX_KITCHEN_DURATION_NIGHT) 
         {
              if (IsDayOrNight(getEpochTime()) == false)
              {
                 publishAlert("Kitchen", "Unusual Use of Kitchen detected");
              }
         }
      }
}