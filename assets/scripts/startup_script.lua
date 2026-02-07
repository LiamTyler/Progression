
local startTime = nil

function Start()
    startTime = Time.GetTimePoint()
end

function Update()
    local dt = Time.GetTimeSince( startTime )
    if dt > 1 then
        local s = LoadScene( "main_menu" )
        SetPrimaryScene( s )
    end
end