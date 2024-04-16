#include "drawing_primitives.h"

#include <functional>
#include <utility>
#include <vector>

#include "line.h"
#include "point.h"
#include "rng.h"

void draw_line( const std::function<void( const point & )> &set, const point &p1, const point &p2 )
{
    line_through_2( p1, p2,
        [&set]( std::vector<point> & new_line ) {
        set( new_line.back() );
        return true;
    } );
}

void draw_square( const std::function<void( const point & )> &set, point p1, point p2 )
{
    if( p1.x > p2.x ) {
        std::swap( p1.x, p2.x );
    }
    if( p1.y > p2.y ) {
        std::swap( p1.y, p2.y );
    }
    for( int x = p1.x; x <= p2.x; x++ ) {
        for( int y = p1.y; y <= p2.y; y++ ) {
            set( point( x, y ) );
        }
    }
}

void draw_rough_circle( const std::function<void( const point & )> &set, const point &p, int rad )
{
    for( int i = p.x - rad; i <= p.x + rad; i++ ) {
        for( int j = p.y - rad; j <= p.y + rad; j++ ) {
            if( trig_dist( p, point( i, j ) ) + rng( 0, 3 ) <= rad ) {
                set( point( i, j ) );
            }
        }
    }
}

void draw_circle( const std::function<void( const point & )> &set, const rl_vec2d &p, double rad )
{
    for( int i = p.x - rad - 1; i <= p.x + rad + 1; i++ ) {
        for( int j = p.y - rad - 1; j <= p.y + rad + 1; j++ ) {
            if( ( p.x - i ) * ( p.x - i ) + ( p.y - j ) * ( p.y - j ) <= rad * rad ) {
                set( point( i, j ) );
            }
        }
    }
}

void draw_circle( const std::function<void( const point & )> &set, const point &p, int rad )
{
    for( int i = p.x - rad; i <= p.x + rad; i++ ) {
        for( int j = p.y - rad; j <= p.y + rad; j++ ) {
            if( trig_dist( p, point( i, j ) ) <= rad ) {
                set( point( i, j ) );
            }
        }
    }
}
