
velocity     = nil
currentSpeed = 3
regularSpeed = 3
boostSpeed   = 9
turnSpeed    = 0.002
maxAngle     = 85 * 3.14159/180
camera       = nil
active       = true

function Start()
    camera = scene.camera
    velocity = vec3.new( 0 )
end

function Update()
    if Input.GetKeyUp( Key.X ) then
        active = not active
    end
    if Input.GetKeyUp( Key.L ) then
        GetMainWindow():SetRelativeMouse( not GetMainWindow():IsRelativeMouse() )
    end
    if not active then
        return
    end

    if Input.GetMouseButtonDown( MouseButton.LEFT ) then
        velocity.y = 1
    end
    if Input.GetMouseButtonDown( MouseButton.RIGHT ) then
        velocity.y = -1
    end
    if Input.GetKeyDown( Key.A ) then
        velocity.x = -1
    end
    if Input.GetKeyDown( Key.D ) then
        velocity.x = 1
    end
    if Input.GetKeyDown( Key.W ) then
        velocity.z = 1
    end
    if Input.GetKeyDown( Key.S ) then
        velocity.z = -1
    end
    if Input.GetKeyDown( Key.LEFT_SHIFT ) then
        currentSpeed = boostSpeed
    end
    
    if Input.GetMouseButtonUp( MouseButton.LEFT ) and velocity.y == 1 then
        velocity.y = 0
    end
    if Input.GetMouseButtonUp( MouseButton.RIGHT ) and velocity.y == -1 then
        velocity.y = 0
    end
    if Input.GetKeyUp( Key.A ) and velocity.x == -1 then
        velocity.x = 0
    end
    if Input.GetKeyUp( Key.D ) and velocity.x == 1 then
        velocity.x = 0
    end
    if Input.GetKeyUp( Key.W ) and velocity.z == 1 then
        velocity.z = 0
    end
    if Input.GetKeyUp( Key.S ) and velocity.z == -1 then
        velocity.z = 0
    end
    
    if Input.GetKeyUp( Key.LEFT_SHIFT ) then
        currentSpeed = regularSpeed
    end
    
    local dMouse    = -Input:GetMouseChange()
    local dRotation = vec3.new( dMouse.y, dMouse.x, 0 )

    camera.rotation   = camera.rotation + vec3.scale( turnSpeed, dRotation )
    camera.rotation.x = math.max( -maxAngle, math.min( maxAngle, camera.rotation.x ) )
    
    camera:UpdateOrientationVectors()
    
    local forward = camera:GetForwardDir()
    local right = camera:GetRightDir()
    local up = camera:GetUpDir()
    local step = currentSpeed * Time.dt
    camera.position = camera.position + vec3.scale( velocity.z * step, forward ) + vec3.scale( velocity.x * step, right ) + vec3.scale( velocity.y * step, up )
    camera:UpdateViewMatrix()
end