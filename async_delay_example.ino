//Don't just copy-paste, for it's just a poc
//and it's tricky

class Wait
{
    public:
    Wait() {curr_time = millis();}
    //update time
    void update() {curr_time = millis();}
    //have [time] passed since last update?
    bool dur(long time) {return (millis() - curr_time >= time);}
    private:
    long curr_time;
};

Wait mywait;

//the code in the if block is executed every 300ms
void loop()
{
    if (mywait.dur(300))
    {
        //do something
        mywait.update();
    } 
}
