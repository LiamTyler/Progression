
velocity       = nil
rotation       = nil
regularSpeed   = 3
boostSpeed     = 15
currentSpeed   = regularSpeed
turnSpeed      = 0.002
maxAngle       = 89 * 3.14159/180
camera         = nil
active         = true
callbackHandle = nil

function InputCallback( cInput )
    velocity.z = 0
    
    velocity.x = cInput:GetAxisValue( Input.Axis.CAMERA_VEL_X )
    velocity.y = cInput:GetAxisValue( Input.Axis.CAMERA_VEL_Y )
    velocity.z = cInput:GetAxisValue( Input.Axis.CAMERA_VEL_Z )
    
    rotation.x = -cInput:GetAxisValue( Input.Axis.CAMERA_PITCH )
    rotation.z = -cInput:GetAxisValue( Input.Axis.CAMERA_YAW )
    
    if cInput:ActionJustPressed( Input.Action.TOGGLE_CAMERA_CONTROLS ) then
        active = not active
        GetMainWindow():SetRelativeMouse( active )        
    end
    
    if cInput:ActionBeingPressed( Input.Action.CAMERA_SPRINT ) then
        currentSpeed = boostSpeed
    end
end

function Start()
    camera = scene.camera
    velocity = vec3.new( 0 )
    rotation = vec3.new( 0 )
    
    callbackHandle = Input.AddCallback( Input.Context.CAMERA_CONTROLS, InputCallback )
end

function End()
    Input.RemoveCallback( Input.Context.CAMERA_CONTROLS, callbackHandle )
end

function Update()
    if not active then
        return
    end

    camera.rotation   = camera.rotation + vec3.scale( turnSpeed, rotation )
    camera.rotation.x = math.max( -maxAngle, math.min( maxAngle, camera.rotation.x ) )
    rotation.x = 0
    rotation.z = 0
    
    camera:UpdateOrientationVectors()
    
    local forward = camera:GetForwardDir()
    local right = camera:GetRightDir()
    local up = camera:GetUpDir()
    local step = currentSpeed * Time.dt
    camera.position = camera.position + vec3.scale( velocity.y * step, forward ) + vec3.scale( velocity.x * step, right ) + vec3.scale( velocity.z * step, up )
    camera:Update()
    
    currentSpeed = regularSpeed
end