/*
    Copyright 2005-2013 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks.

    Threading Building Blocks is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    version 2 as published by the Free Software Foundation.

    Threading Building Blocks is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Threading Building Blocks; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    As a special exception, you may use this file as part of a free software
    library without restriction.  Specifically, if other files instantiate
    templates or use macros or inline functions from this file, or you compile
    this file and link it with other files to produce an executable, this
    file does not by itself cause the resulting executable to be covered by
    the GNU General Public License.  This exception does not however
    invalidate any other reasons why the executable file might be covered by
    the GNU General Public License.
*/

#ifndef __TBB_SERIAL_parallel_for_H
#define __TBB_SERIAL_parallel_for_H

#if !TBB_USE_EXCEPTIONS && _MSC_VER
    // Suppress "C++ exception handler used, but unwind semantics are not enabled" warning in STL headers
    #pragma warning (push)
    #pragma warning (disable: 4530)
#endif

#include <stdexcept>
#include <string> // required to construct std exception classes

#if !TBB_USE_EXCEPTIONS && _MSC_VER
    #pragma warning (pop)
#endif

#include "tbb_annotate.h"

#ifndef __TBB_NORMAL_EXECUTION
#include "tbb/blocked_range.h"
#include "tbb/partitioner.h"
#endif

namespace tbb {
namespace serial {
namespace interface6 {

// parallel_for serial annotated implementation

template< typename Range, typename Body, typename Partitioner >
class start_for : tbb::internal::no_copy {
    Range my_range;
    const Body my_body;
    typename Partitioner::task_partition_type my_partition;
    void execute();

    //! Constructor for root task.
    start_for( const Range& range, const Body& body, Partitioner& partitioner ) :
        my_range( range ),
        my_body( body ),
        my_partition( partitioner )
    {
    }

    //! Splitting constructor used to generate children.
    /** this becomes left child.  Newly constructed object is right child. */
    start_for( start_for& parent_, split ) :
        my_range( parent_.my_range, split() ),
        my_body( parent_.my_body ),
        my_partition( parent_.my_partition, split() )
    {
    }

public:
    static void run(  const Range& range, const Body& body, Partitioner& partitioner ) {
        if( !range.empty() ) {
            ANNOTATE_SITE_BEGIN( tbb_parallel_for );
            {
                start_for a( range, body, partitioner );
                a.execute();
            }
            ANNOTATE_SITE_END( tbb_parallel_for );
        }
    }
};

template< typename Range, typename Body, typename Partitioner >
void start_for< Range, Body, Partitioner >::execute() {
    if( !my_range.is_divisible() || !my_partition.divisions_left() ) {
        ANNOTATE_TASK_BEGIN( tbb_parallel_for_range );
        {
            my_body( my_range );
        }
        ANNOTATE_TASK_END( tbb_parallel_for_range );
    } else {
        start_for b( *this, split() );
        this->execute(); // Execute the left interval first to keep the serial order.
        b.execute();     // Execute the right interval then.
    }
}

//! Parallel iteration over range with default partitioner.
/** @ingroup algorithms **/
template<typename Range, typename Body>
void parallel_for( const Range& range, const Body& body ) {
    serial::interface6::start_for<Range,Body,const __TBB_DEFAULT_PARTITIONER>::run(range,body,__TBB_DEFAULT_PARTITIONER());
}

//! Parallel iteration over range with simple partitioner.
/** @ingroup algorithms **/
template<typename Range, typename Body>
void parallel_for( const Range& range, const Body& body, const simple_partitioner& partitioner ) {
    serial::interface6::start_for<Range,Body,const simple_partitioner>::run(range,body,partitioner);
}

//! Parallel iteration over range with auto_partitioner.
/** @ingroup algorithms **/
template<typename Range, typename Body>
void parallel_for( const Range& range, const Body& body, const auto_partitioner& partitioner ) {
    serial::interface6::start_for<Range,Body,const auto_partitioner>::run(range,body,partitioner);
}

//! Parallel iteration over range with affinity_partitioner.
/** @ingroup algorithms **/
template<typename Range, typename Body>
void parallel_for( const Range& range, const Body& body, affinity_partitioner& partitioner ) {
    serial::interface6::start_for<Range,Body,affinity_partitioner>::run(range,body,partitioner);
}

//! Implementation of parallel iteration over stepped range of integers with explicit step and partitioner (ignored)
template <typename Index, typename Function, typename Partitioner>
void parallel_for_impl(Index first, Index last, Index step, const Function& f, Partitioner& ) {
    if (step <= 0 )
        throw std::invalid_argument( "nonpositive_step" );
    else if (last > first) {
        // Above "else" avoids "potential divide by zero" warning on some platforms
        ANNOTATE_SITE_BEGIN( tbb_parallel_for );
        for( Index i = first; i < last; i = i + step ) {
            ANNOTATE_TASK_BEGIN( tbb_parallel_for_iteration );
            { f( i ); }
            ANNOTATE_TASK_END( tbb_parallel_for_iteration );
        }
        ANNOTATE_SITE_END( tbb_parallel_for );
    }
}

//! Parallel iteration over a range of integers with explicit step and default partitioner
template <typename Index, typename Function>
void parallel_for(Index first, Index last, Index step, const Function& f) {
    parallel_for_impl<Index,Function,const auto_partitioner>(first, last, step, f, auto_partitioner());
}
//! Parallel iteration over a range of integers with explicit step and simple partitioner
template <typename Index, typename Function>
void parallel_for(Index first, Index last, Index step, const Function& f, const simple_partitioner& p) {
    parallel_for_impl<Index,Function,const simple_partitioner>(first, last, step, f, p);
}
//! Parallel iteration over a range of integers with explicit step and auto partitioner
template <typename Index, typename Function>
void parallel_for(Index first, Index last, Index step, const Function& f, const auto_partitioner& p) {
    parallel_for_impl<Index,Function,const auto_partitioner>(first, last, step, f, p);
}
//! Parallel iteration over a range of integers with explicit step and affinity partitioner
template <typename Index, typename Function>
void parallel_for(Index first, Index last, Index step, const Function& f, affinity_partitioner& p) {
    parallel_for_impl(first, last, step, f, p);
}

//! Parallel iteration over a range of integers with default step and default partitioner
template <typename Index, typename Function>
void parallel_for(Index first, Index last, const Function& f) {
    parallel_for_impl<Index,Function,const auto_partitioner>(first, last, static_cast<Index>(1), f, auto_partitioner());
}
//! Parallel iteration over a range of integers with default step and simple partitioner
template <typename Index, typename Function>
void parallel_for(Index first, Index last, const Function& f, const simple_partitioner& p) {
    parallel_for_impl<Index,Function,const simple_partitioner>(first, last, static_cast<Index>(1), f, p);
}
//! Parallel iteration over a range of integers with default step and auto partitioner
template <typename Index, typename Function>
    void parallel_for(Index first, Index last, const Function& f, const auto_partitioner& p) {
    parallel_for_impl<Index,Function,const auto_partitioner>(first, last, static_cast<Index>(1), f, p);
}
//! Parallel iteration over a range of integers with default step and affinity_partitioner
template <typename Index, typename Function>
void parallel_for(Index first, Index last, const Function& f, affinity_partitioner& p) {
    parallel_for_impl(first, last, static_cast<Index>(1), f, p);
}

} // namespace interface6

using interface6::parallel_for;

} // namespace serial

#ifndef __TBB_NORMAL_EXECUTION
using serial::interface6::parallel_for;
#endif

} // namespace tbb

#endif /* __TBB_SERIAL_parallel_for_H */
