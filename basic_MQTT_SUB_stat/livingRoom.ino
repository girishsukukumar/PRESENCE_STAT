
void RunAnomalyCheckForLivingRoom()
{
      time_t diff ;
      int idx = findRoomIndex("LivingRoom");
      time_t now = getEpochTime() ;
      

      if (GetOccupancyStateOfRoom("LivingRoom") == true)
      {
        /* Check the duration of occupancy */
         diff = now - GetOccupiedEpocTimeOfRoom("Living");
         if (diff > TrainedData.MAX_LIVINGROOM_DURATION_NIGHT) 
         {
              if (IsDayOrNight(getEpochTime()) == false)
              {
                 publishAlert("LivingRoom", "Unusal  Use of Living room detected at night");
              }
         }
      }
}