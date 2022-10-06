
typedef enum
{
    GUN_TYPE_HANDGUN = 0,
    GUN_TYPE_MAX,
} GunType;

typedef struct
{
    GunType type;

    int bullets;
    int bullets_max;
} Gun;
